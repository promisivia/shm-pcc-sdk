-------------------------- MODULE HelpUpdateLinearizability --------------------------
EXTENDS Integers, Naturals, Sequences

(***************************************************************************)
(* TLA+ 抽象 help_update.h 中的无锁 help-update 算法，                     *)
(* 以线性一致性为目标定义简化模型与不变量。                                 *)
(***************************************************************************)

\* -----------------------------------------------------------------------------
\* 1. 常量与基本约束
\* -----------------------------------------------------------------------------

CONSTANTS NumThreads, Values, InitValue

ASSUME NumThreads \in Nat /\ NumThreads > 0
ASSUME Values \subseteq Int /\ InitValue \in Values

\* -----------------------------------------------------------------------------
\* 2. 位操作工具
\* -----------------------------------------------------------------------------

IsLocked(p) == p % 2 = 1
ClearLock(p) == p - (p % 2)
SetLock(p) == ClearLock(p) + 1
BaseValue == ClearLock(InitValue)
PointerOf(n) == BaseValue + 2 * n
LogicalValue(p) == (ClearLock(p) - BaseValue) \div 2
Max(S) ==
    IF S = {} THEN 0
    ELSE CHOOSE n \in S : \A m \in S : m <= n

LatestValue(seq) ==
    LET idxs == {i \in DOMAIN seq : seq[i].type = "update"}
    IN IF idxs = {} THEN LogicalValue(InitValue)
       ELSE seq[Max(idxs)].data

Monotonic(seq) ==
    \A i, j \in DOMAIN seq : i <= j => seq[i].data <= seq[j].data

Threads == 0..(NumThreads - 1)

\* BEGIN TRANSLATION
\* -----------------------------------------------------------------------------
\* 3. 状态变量
\* -----------------------------------------------------------------------------

VARIABLES
    GlobalPtr,              \* 全局指针（最低位为锁位）
    ReplicaPtrs,            \* 每线程副本
    OpState,                \* 线程控制状态。
    CurUpdateValue,         \* 正在处理的目标值
    StartCheckIdFirst,      \* help-update 当前处理的副本索引
    HelpUpdateCaller,       \* {"NONE","UPDATE","LOAD"}
    ValueCounter,           \* 逻辑值计数器，仿 cosmos.tla 中的 value
    LoadSnapshot,           \* 记录 load 开始时已完成的最新 update 值
    History                 \* 线性化历史（操作记录序列）

Vars == <<GlobalPtr, ReplicaPtrs, OpState,
          CurUpdateValue, StartCheckIdFirst, HelpUpdateCaller,
          ValueCounter, LoadSnapshot, History>>

\* 辅助操作符（在 VARIABLE 声明之后定义）
Replica(i) == ReplicaPtrs[i]
HelpIdx(i) == StartCheckIdFirst[i]
UpdateVal(i) == CurUpdateValue[i]
\* ↑ 映射到 `help_update.h` 中的 `replica_ptrs[i]`、帮助索引与 `CurUpdateValue`。

Init ==
    /\ GlobalPtr = ClearLock(InitValue)
    /\ ReplicaPtrs = [i \in Threads |-> ClearLock(InitValue)]
    /\ OpState = [i \in Threads |-> "IDLE"]
    /\ CurUpdateValue = [i \in Threads |-> InitValue]
    /\ StartCheckIdFirst = [i \in Threads |-> 0]
    /\ HelpUpdateCaller = [i \in Threads |-> "NONE"]
    /\ ValueCounter = LogicalValue(InitValue)
    /\ LoadSnapshot = [i \in Threads |-> LogicalValue(InitValue)]
    /\ History = <<>>

OpRecord == [type : {"update","load"}, tid : Threads, data : Int, min : Int]

(*--algorithm HelpUpdateAlgo
variables
    GlobalPtr = ClearLock(InitValue),
    ReplicaPtrs = [i \in Threads |-> ClearLock(InitValue)],
    OpState = [i \in Threads |-> "IDLE"],
    CurUpdateValue = [i \in Threads |-> InitValue],
    StartCheckIdFirst = [i \in Threads |-> 0],
    HelpUpdateCaller = [i \in Threads |-> "NONE"],
    ValueCounter = LogicalValue(InitValue),
    LoadSnapshot = [i \in Threads |-> LogicalValue(InitValue)],
    History = <<>>;

macro record_update(tid, val) begin
    History := Append(History,
        [type |-> "update", tid |-> tid,
         data |-> LogicalValue(val),
         min |-> LogicalValue(val)]);
end macro;

macro record_load(tid, val) begin
    History := Append(History,
        [type |-> "load", tid |-> tid,
         data |-> LogicalValue(ClearLock(val)),
         min |-> LoadSnapshot[tid]]);
end macro;

process (thread \in Threads)
variable idx = 0, local_r = 0, cleared = 0, cur_global = 0, old_val = 0;
begin ThreadIdle:
    OpState[self] := "IDLE";
ChooseOperation:
    \* either update_ptr（help_update.h L124-L164）或 load_ptr（L166-L182）入口
    either
        StartUpdate:
            \* ValueCounter 是全局共享变量，在 TLA+ 中每个动作是原子的
            \* 所以每个线程在执行递增操作时都会看到最新的值，并发修改是可见的
            ValueCounter := ValueCounter + 1;
            CurUpdateValue[self] := PointerOf(ValueCounter);
            StartCheckIdFirst[self] := 0;
            HelpUpdateCaller[self] := "NONE";
            OpState[self] := "UP_RESTART";
            goto UpdatePtr;
    or
        StartLoad:
            LoadSnapshot[self] := LatestValue(History);
            OpState[self] := "LD_CHECK";
            goto LoadPtr;
    end either;

UpdatePtr:
    \* 对应 cas_ptr 函数（help_update.h L126-L163）。
    \* 尝试获取全局指针的锁，如果已有锁则先帮助完成
    cur_global := GlobalPtr;
    if IsLocked(cur_global) then
        \* 如果当前有锁位，先帮助更新（help_update.h L130-L132）。
        CurUpdateValue[self] := ClearLock(cur_global);
        StartCheckIdFirst[self] := 0;
        HelpUpdateCaller[self] := "UPDATE";
        OpState[self] := "HELP_RESTART";
        goto HelpUpdate;
    end if;
    
    \* 保存期望的旧值用于 CAS 检查（help_update.h L135）。
    \* 如果从 HelpUpdate 返回，cur_global 应该等于 CurUpdateValue[self]（help_update 的返回值）
    \* 为了准确模拟 C++ 代码，我们使用 cur_global 作为 old_val
    old_val := cur_global;
    
    \* 步骤1: CAS全局指针为new_val.1（help_update.h L135-L138）。
    if GlobalPtr = old_val then
        GlobalPtr := SetLock(CurUpdateValue[self]);
        \* CAS成功（commit point，help_update.h L139）
        OpState[self] := "UP_LOOP";
        StartCheckIdFirst[self] := 0;
        goto UpdateReplicas;
    else
        \* CAS失败，直接返回false交给外面的逻辑处理（help_update.h L136-L137）。
        \* 不记录 history，直接返回 idle，然后可以重新选择 update 或 load
        OpState[self] := "IDLE";
        HelpUpdateCaller[self] := "NONE";
        StartCheckIdFirst[self] := 0;
        goto ThreadIdle;
    end if;

UpdateReplicas:
    \* 对应 update_ptr 中遍历 replica_ptrs 并逐一 CAS（help_update.h L142-L156）。
    if StartCheckIdFirst[self] < NumThreads then
        idx := StartCheckIdFirst[self];
        local_r := ReplicaPtrs[idx];
        \* fast check（help_update.h L146-L148）。
        if ClearLock(local_r) = CurUpdateValue[self] then
            StartCheckIdFirst[self] := idx + 1;
            goto UpdateReplicas;
        else
            \* current_cas（help_update.h L150-L154）。
            if local_r = ReplicaPtrs[idx] then
                ReplicaPtrs[idx] := SetLock(CurUpdateValue[self]);
                StartCheckIdFirst[self] := idx + 1;
                goto UpdateReplicas;
            else
                \* CAS失败，且不是因为有人成功更新了replica；说明global_ptr被修改了
                \* 但只可能是因为这一轮的global已经成功更新，但又被改了，还是可以返回true
                \* （help_update.h L151-L153）。在 TLA+ 中，我们继续完成操作
                StartCheckIdFirst[self] := idx + 1;
                goto UpdateReplicas;
            end if;
        end if;
    else
        \* 步骤3: 所有副本都更新完成，清除G的锁位 (写G为A.0)（help_update.h L159-L162）。
        cur_global := GlobalPtr;
        if ClearLock(cur_global) = CurUpdateValue[self] then
            \* 使用 current_cas 清除锁位（help_update.h L159）。
            if cur_global = GlobalPtr then
                GlobalPtr := CurUpdateValue[self];
            end if;
            \* CAS失败，且不是因为有人提前清理了global_ptr；说明global_ptr被修改了
            \* 但只可能是因为这一轮的global已经成功更新，但又被改了，还是可以返回true
            \* （help_update.h L160-L162）。在 TLA+ 中，我们完成操作
            OpState[self] := "IDLE";
            HelpUpdateCaller[self] := "NONE";
            StartCheckIdFirst[self] := 0;
            record_update(self, CurUpdateValue[self]);
            goto ThreadIdle;
        else
            \* global_ptr 的值不匹配，但操作已经成功（因为我们已经更新了所有副本）
            \* 继续完成操作
            OpState[self] := "IDLE";
            HelpUpdateCaller[self] := "NONE";
            StartCheckIdFirst[self] := 0;
            record_update(self, CurUpdateValue[self]);
            goto ThreadIdle;
        end if;
    end if;

LoadPtr:
    \* 对应 load_ptr（help_update.h L171-L182）。
    \* 检查当前线程的副本指针
    local_r := ReplicaPtrs[self];
    \* Case (1): 如果R[i]以.0结尾，直接返回（help_update.h L176-L178）。
    if ~IsLocked(local_r) then
        OpState[self] := "IDLE";
        record_load(self, local_r);
        goto ThreadIdle;
    else
        \* Case (2): 如果R[i]以.1结尾，帮助更新R[i+1]到R[N]为A.1（help_update.h L181）。
        CurUpdateValue[self] := ClearLock(local_r);
        StartCheckIdFirst[self] := self + 1;
        HelpUpdateCaller[self] := "LOAD";
        OpState[self] := "HELP_RESTART";
        goto HelpUpdate;
    end if;

HelpUpdate:
    \* 对应 help_update 的 restart_global 标签（help_update.h L74）。
    \* 首先检查global_ptr是否没有锁位且没有被修改，如果满足则返回（help_update.h L78-L81）。
    cur_global := GlobalPtr;
    if ClearLock(cur_global) = CurUpdateValue[self] /\ ~IsLocked(cur_global) then
        \* 成功条件满足，返回
        if HelpUpdateCaller[self] = "LOAD" then
            OpState[self] := "IDLE";
            HelpUpdateCaller[self] := "NONE";
            StartCheckIdFirst[self] := 0;
            record_load(self, CurUpdateValue[self]);
            goto ThreadIdle;
        else
            \* 从UPDATE调用，help_update 成功完成，返回清除锁位后的值
            \* 在 C++ 代码中，help_update 返回的值会被赋值给 cur_global
            \* 在 TLA+ 中，CurUpdateValue[self] 已经是清除锁位后的值
            \* 返回到 UpdatePtr 继续 CAS
            OpState[self] := "UP_RESTART";
            StartCheckIdFirst[self] := 0;
            HelpUpdateCaller[self] := "NONE";
            goto UpdatePtr;
        end if;
    else
        \* 需要继续帮助更新，进入循环
        OpState[self] := "HELP_LOOP";
        goto HelpUpdateReplicas;
    end if;

HelpUpdateReplicas:
    \* 对应 help_update 的 for-loop（help_update.h L76-L108）。
    \* 遍历副本并帮助更新
    if StartCheckIdFirst[self] < NumThreads then
        idx := StartCheckIdFirst[self];
        \* 首先检查global_ptr是否满足条件（help_update.h L78-L81）。
        cur_global := GlobalPtr;
        if ClearLock(cur_global) = CurUpdateValue[self] /\ ~IsLocked(cur_global) then
            \* 成功条件满足，返回
            if HelpUpdateCaller[self] = "LOAD" then
                OpState[self] := "IDLE";
                HelpUpdateCaller[self] := "NONE";
                StartCheckIdFirst[self] := 0;
                record_load(self, CurUpdateValue[self]);
                goto ThreadIdle;
            else
                OpState[self] := "UP_RESTART";
                StartCheckIdFirst[self] := 0;
                HelpUpdateCaller[self] := "NONE";
                goto UpdatePtr;
            end if;
        end if;
        \* 然后尝试依次更新每个replica（help_update.h L84-L107）。
        local_r := ReplicaPtrs[idx];
        \* 可以假设有人在一直更新，可以先check是不是已经更新成功了（help_update.h L87-L89）。
        if ClearLock(local_r) = CurUpdateValue[self] then
            StartCheckIdFirst[self] := idx + 1;
            goto HelpUpdateReplicas;
        else
            \* 需要帮助更新，但必须先判断是不是因为global变了导致的这个replica被修改（help_update.h L92-L97）。
            cur_global := GlobalPtr;
            if ClearLock(cur_global) # CurUpdateValue[self] then
                \* 立刻返回失败，需要从新的value开始（help_update.h L94-L96）。
                CurUpdateValue[self] := ClearLock(cur_global);
                StartCheckIdFirst[self] := 0;
                OpState[self] := "HELP_RESTART";
                goto HelpUpdate;
            else
                \* 不是因为global变了，需要帮助更新（help_update.h L99-L106）。
                if local_r = ReplicaPtrs[idx] then
                    ReplicaPtrs[idx] := SetLock(CurUpdateValue[self]);
                    StartCheckIdFirst[self] := idx + 1;
                    goto HelpUpdateReplicas;
                else
                    \* CAS失败，重新开始（help_update.h L103-L105）。
                    CurUpdateValue[self] := ClearLock(GlobalPtr);
                    StartCheckIdFirst[self] := 0;
                    OpState[self] := "HELP_RESTART";
                    goto HelpUpdate;
                end if;
            end if;
        end if;
    else
        \* 这一轮检查，所有的replica和global的值都更新为expected_val了（help_update.h L111-L116）。
        \* 尝试清除global_ptr的锁位
        cur_global := GlobalPtr;
        if ClearLock(cur_global) = CurUpdateValue[self] then
            if cur_global = GlobalPtr then
                GlobalPtr := CurUpdateValue[self];
                \* 成功返回
                if HelpUpdateCaller[self] = "LOAD" then
                    OpState[self] := "IDLE";
                    HelpUpdateCaller[self] := "NONE";
                    StartCheckIdFirst[self] := 0;
                    record_load(self, CurUpdateValue[self]);
                    goto ThreadIdle;
                else
                    OpState[self] := "UP_RESTART";
                    StartCheckIdFirst[self] := 0;
                    HelpUpdateCaller[self] := "NONE";
                    goto UpdatePtr;
                end if;
            else
                \* CAS失败，重新开始（help_update.h L118-L121）。
                CurUpdateValue[self] := ClearLock(GlobalPtr);
                StartCheckIdFirst[self] := 0;
                OpState[self] := "HELP_RESTART";
                goto HelpUpdate;
            end if;
        else
            \* global_ptr被其他人更新了，重新开始（help_update.h L119-L121）。
            CurUpdateValue[self] := ClearLock(GlobalPtr);
            StartCheckIdFirst[self] := 0;
            OpState[self] := "HELP_RESTART";
            goto HelpUpdate;
        end if;
    end if;
end process;
end algorithm;
*)

\* PlusCal 翻译器会在上面自动插入翻译后的代码，包括：
\* - 所有标签对应的动作（如 StartUpdate, StartLoad, UpdatePtr 等）
\* - Next 操作符
\* - Spec 操作符（定义为 Init /\ [][Next]_vars）
\* 
\* 如果翻译器还没有运行，下面的临时定义会生效。
\* 如果翻译器已经运行，翻译后的代码会覆盖这些定义。

\* 临时定义（如果 PlusCal 翻译器已运行，这些会被覆盖）
\* 这是一个简化的 Next 定义，基于 OpState 的变化
\* 实际运行时，PlusCal 翻译器会生成更精确的动作定义

StateTransition(i) ==
    ((OpState[i] = "IDLE" /\ OpState'[i] \in {"UP_RESTART", "LD_CHECK"}))
    \/ ((OpState[i] = "UP_RESTART" /\ OpState'[i] \in {"UP_LOOP", "HELP_RESTART", "UP_RESTART"}))
    \/ ((OpState[i] = "UP_LOOP" /\ OpState'[i] \in {"UP_LOOP", "UP_RESTART", "IDLE"}))
    \/ ((OpState[i] = "LD_CHECK" /\ OpState'[i] \in {"IDLE", "HELP_RESTART"}))
    \/ ((OpState[i] = "HELP_RESTART" /\ OpState'[i] \in {"HELP_LOOP", "UP_RESTART", "LD_CHECK", "IDLE"}))
    \/ ((OpState[i] = "HELP_LOOP" /\ OpState'[i] \in {"HELP_LOOP", "HELP_RESTART", "UP_RESTART", "IDLE"}))

Next == 
    ((\E i \in Threads : StateTransition(i)) /\ UNCHANGED <<ValueCounter, LoadSnapshot, History>>)
    \/ 
    ((\A i \in Threads : OpState'[i] = OpState[i]) /\ 
     (GlobalPtr' # GlobalPtr \/ ReplicaPtrs' # ReplicaPtrs \/ 
      CurUpdateValue' # CurUpdateValue \/ StartCheckIdFirst' # StartCheckIdFirst \/
      HelpUpdateCaller' # HelpUpdateCaller \/ ValueCounter' # ValueCounter \/
      LoadSnapshot' # LoadSnapshot \/ History' # History))

Spec == Init /\ [][Next]_Vars

\* -----------------------------------------------------------------------------
\* 6. 线性一致性不变量
\* -----------------------------------------------------------------------------

Linearizability ==
    \A i \in DOMAIN History :
        History[i].type = "load" => History[i].data >= History[i].min

\* -----------------------------------------------------------------------------
\* 7. 类型不变量
\* -----------------------------------------------------------------------------

TypeInvariant ==
    /\ GlobalPtr \in Int
    /\ ReplicaPtrs \in [Threads -> Int]
    /\ OpState \in [Threads ->
        {"IDLE","UP_RESTART","UP_LOOP","LD_CHECK","HELP_RESTART","HELP_LOOP"}]
    /\ CurUpdateValue \in [Threads -> Int]
    /\ StartCheckIdFirst \in [Threads -> 0..NumThreads]
    /\ HelpUpdateCaller \in [Threads -> {"NONE","UPDATE","LOAD"}]
    /\ ValueCounter \in Int
    /\ LoadSnapshot \in [Threads -> Int]
    /\ History \in Seq(OpRecord)

==============================================================================

\* END TRANSLATION

\* -----------------------------------------------------------------------------
\* 8. 定理（在 END TRANSLATION 之后，可以使用 PlusCal 翻译后生成的 Spec）
\* -----------------------------------------------------------------------------

THEOREM TypeSafety == Spec => []TypeInvariant

THEOREM Linearizable == Spec => []Linearizability


-------------------------- MODULE help_update --------------------------
EXTENDS Naturals, Integers, Reals, Sequences, FiniteSets, TLC

CONSTANT NumThreads  (* 副本指针的数量 (NumThreads > 0) *)
ASSUME NumThreads \in Nat /\ NumThreads > 0

CONSTANT MaxNumOp

Operations == [type: {"write"}, data: Nat, thread: 0..NumThreads-1]
       \union [type: {"read"}, data: Nat, thread: 0..NumThreads-1]

Threads == 0..NumThreads-1

(*
--algorithm HelpUpdateMechanism
{
variables
(* Global pointer value (simulates clear_lock_bit(global_ptr))*)
G_val = 0;              
(* Global pointer lock bit (simulates has_lock_bit(global_ptr))*)
G_locked = FALSE;       
(* Value to be written *)
value = 1;
(* Replica pointer values *)
R_val = [i \in Threads |-> 0]; 
(* Replica pointer lock bits *)
R_locked = [i \in Threads |-> FALSE];
(* Read result *)
read_result = [i \in Threads |-> 0];
(* Write result *)
write_result = [i \in Threads |-> FALSE];
(* Help update result *)
help_update_result = [i \in Threads |-> 0];
(* History *)
History = <<>>;

(* HelpUpdate procedure *)
procedure HelpUpdate(expected_val, start_check_id) 
variable ii = 0, cur_G_val = 0, local_R_val = 0; \* 局部变量：ii, cur_G_val, local_R_val
{
restart_global:
    (* 检查全局指针是否已完成更新 (cur_global_ptr = expected_val.0) *)
    cur_G_val := G_val;
    if (cur_G_val = expected_val /\ G_locked = FALSE) {
        help_update_result[self] := expected_val; \* 成功返回
HP0:
        return;
    };
    (* 依次帮助更新每个 Replica *)
HP1:
    ii := start_check_id;
HP2:
    while (ii < NumThreads) {
        (* 检查全局状态，如果已完成或改变，则退出/重启 *)
        if (G_val = expected_val /\ G_locked = FALSE) {
            help_update_result[self] := expected_val; \* 成功返回
            return;
        };
        
        (* 然后尝试依次更新每个 replica *)
HP3:
        local_R_val := R_val[ii];
        
        (* 可以假设有人在一直更新，可以先check是不是已经更新成功了 *)
        (* 注意：在 PlusCal 中，replica 的 lock bit 始终为 FALSE，因为它不承担 Writer 的 commit 锁。 *)
        if (local_R_val /= expected_val) {
            (* 需要帮助更新，但必须先判断是不是因为 global 变了 *)
            cur_G_val := G_val;
            if (cur_G_val /= expected_val) {
                (* 立刻返回失败，需要从新的 value 开始 (goto restart_global) *)
                expected_val := cur_G_val;
                start_check_id := 0;
                goto restart_global;
            } else {
                (* 不是因为 global 变了，说明这个 replica 更旧，需要帮助更新 *)
                R_val[ii] := expected_val;
                R_locked[ii] := TRUE; \* 模拟 set_lock_bit(expected_val)
            }
        };
HP4:
        ii := ii + 1;
    };
    
    (* 这一轮检查，所有的 replica 和 global 的值都更新为 expected_val (带锁位) 了 *)
    if (G_val = expected_val) {
        (* 尝试 CAS G 从 locked 到 unlocked (current_cas(global_ptr, cur_global_ptr, expected_val)) *)
        if (G_locked = TRUE) {
            G_locked := FALSE; \* CAS 成功 (清除锁位)
        };
        help_update_result[self] := G_val; \* 成功返回
HP5:
        return;
    } else {
        (* G_val 改变了 *)
        expected_val := G_val;
        start_check_id := 0;
        goto restart_global;
    }
};

(* Writer *)
procedure writer(old_val, new_val)
variable iii = 0;
{
W0:
    (* 0: 如果当前有锁位，先帮助更新 (等待其他更新完成) *)
    if (G_locked = TRUE) {
        call HelpUpdate(G_val, 0); (* HelpUpdate 返回后 G_locked 应该为 FALSE *)
    };
W1:
    (* 1: CAS 全局指针为 new_val.1 (设置锁位) *)
    if (G_val /= old_val) {
        write_result[self] := FALSE; (* CAS 失败，返回 false *)
W2:
        return;
    }; 

W3: 
    (* 2: CAS 成功 (Commit Point: 锁定) *)
    G_locked := TRUE; 
    G_val := new_val;

    (* 3: 依次写每个R[i]为new_val.1 *)
    iii := 0;
W4:
    while (iii < NumThreads) {
W5:
        R_locked[iii] := TRUE;
        R_val[iii] := new_val;
        iii := iii + 1;
    };
W6:
    write_result[self] := TRUE;
    History := Append(History, [type |-> "write",
                            data |-> new_val,
                            thread |-> self]);
W7:
    G_locked := FALSE;
W8:
    return;
}

procedure reader()
{
R0: (* 1: 如果R[i]没有锁位，直接读取R[i]的值 *)
    if (R_locked[self] = FALSE) {
        read_result[self] := R_val[self];
        History := Append(History, [type |-> "read",
                                data |-> read_result[self],
                                thread |-> self]);
    } else {
R1: (* 2: 如果R[i]有锁位，先帮助更新R[i]的值 *)
        call HelpUpdate(R_val[self], self + 1); (* HelpUpdate 返回最终 clean value G_val *)
R2:
        read_result[self] := help_update_result[self];
        History := Append(History, [type |-> "read",
                                data |-> read_result[self],
                                thread |-> self]);
    };
R3:
    return;
}

(* -------------------------------------------------------------- *)
(* --------------------- THREAD ACTIONS ------------------------- *)
(* -------------------------------------------------------------- *)
fair process (thread_id \in Threads)
variables num_op = 0;
{
    thread_loop:
    while (num_op < MaxNumOp) {
        num_op := num_op + 1;
        either {
        read:
            call reader();
        } or {
        write:
            call writer(G_val, G_val + 1);
        }
    }
}
}*) (* END of algorithm *)

\* BEGIN TRANSLATION
CONSTANT defaultInitValue
VARIABLES G_val, G_locked, value, R_val, R_locked, read_result, write_result, 
          help_update_result, History, pc, stack, expected_val, 
          start_check_id, ii, cur_G_val, local_R_val, old_val, new_val, iii, 
          num_op

vars == << G_val, G_locked, value, R_val, R_locked, read_result, write_result, 
           help_update_result, History, pc, stack, expected_val, 
           start_check_id, ii, cur_G_val, local_R_val, old_val, new_val, iii, 
           num_op >>

ProcSet == (Threads)

Init == (* Global variables *)
        /\ G_val = 0
        /\ G_locked = FALSE
        /\ value = 1
        /\ R_val = [i \in Threads |-> 0]
        /\ R_locked = [i \in Threads |-> FALSE]
        /\ read_result = [i \in Threads |-> 0]
        /\ write_result = [i \in Threads |-> FALSE]
        /\ help_update_result = [i \in Threads |-> 0]
        /\ History = <<>>
        (* Procedure HelpUpdate *)
        /\ expected_val = [ self \in ProcSet |-> defaultInitValue]
        /\ start_check_id = [ self \in ProcSet |-> defaultInitValue]
        /\ ii = [ self \in ProcSet |-> 0]
        /\ cur_G_val = [ self \in ProcSet |-> 0]
        /\ local_R_val = [ self \in ProcSet |-> 0]
        (* Procedure writer *)
        /\ old_val = [ self \in ProcSet |-> defaultInitValue]
        /\ new_val = [ self \in ProcSet |-> defaultInitValue]
        /\ iii = [ self \in ProcSet |-> 0]
        (* Process thread_id *)
        /\ num_op = [self \in Threads |-> 0]
        /\ stack = [self \in ProcSet |-> << >>]
        /\ pc = [self \in ProcSet |-> "thread_loop"]

restart_global(self) == /\ pc[self] = "restart_global"
                        /\ cur_G_val' = [cur_G_val EXCEPT ![self] = G_val]
                        /\ IF cur_G_val'[self] = expected_val[self] /\ G_locked = FALSE
                              THEN /\ help_update_result' = [help_update_result EXCEPT ![self] = expected_val[self]]
                                   /\ pc' = [pc EXCEPT ![self] = "HP0"]
                              ELSE /\ pc' = [pc EXCEPT ![self] = "HP1"]
                                   /\ UNCHANGED help_update_result
                        /\ UNCHANGED << G_val, G_locked, value, R_val, 
                                        R_locked, read_result, write_result, 
                                        History, stack, expected_val, 
                                        start_check_id, ii, local_R_val, 
                                        old_val, new_val, iii, num_op >>

HP0(self) == /\ pc[self] = "HP0"
             /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
             /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
             /\ cur_G_val' = [cur_G_val EXCEPT ![self] = Head(stack[self]).cur_G_val]
             /\ local_R_val' = [local_R_val EXCEPT ![self] = Head(stack[self]).local_R_val]
             /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
             /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
             /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
             /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                             read_result, write_result, help_update_result, 
                             History, old_val, new_val, iii, num_op >>

HP1(self) == /\ pc[self] = "HP1"
             /\ ii' = [ii EXCEPT ![self] = start_check_id[self]]
             /\ pc' = [pc EXCEPT ![self] = "HP2"]
             /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                             read_result, write_result, help_update_result, 
                             History, stack, expected_val, start_check_id, 
                             cur_G_val, local_R_val, old_val, new_val, iii, 
                             num_op >>

HP2(self) == /\ pc[self] = "HP2"
             /\ IF ii[self] < NumThreads
                   THEN /\ IF G_val = expected_val[self] /\ G_locked = FALSE
                              THEN /\ help_update_result' = [help_update_result EXCEPT ![self] = expected_val[self]]
                                   /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
                                   /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
                                   /\ cur_G_val' = [cur_G_val EXCEPT ![self] = Head(stack[self]).cur_G_val]
                                   /\ local_R_val' = [local_R_val EXCEPT ![self] = Head(stack[self]).local_R_val]
                                   /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
                                   /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
                                   /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
                              ELSE /\ pc' = [pc EXCEPT ![self] = "HP3"]
                                   /\ UNCHANGED << help_update_result, stack, 
                                                   expected_val, 
                                                   start_check_id, ii, 
                                                   cur_G_val, local_R_val >>
                        /\ UNCHANGED G_locked
                   ELSE /\ cur_G_val' = [cur_G_val EXCEPT ![self] = G_val]
                        /\ IF G_val = expected_val[self]
                              THEN /\ IF G_locked = TRUE
                                         THEN /\ G_locked' = FALSE
                                         ELSE /\ TRUE
                                              /\ UNCHANGED G_locked
                                   /\ help_update_result' = [help_update_result EXCEPT ![self] = G_val]
                                   /\ pc' = [pc EXCEPT ![self] = "HP5"]
                                   /\ UNCHANGED << expected_val, 
                                                   start_check_id >>
                              ELSE /\ expected_val' = [expected_val EXCEPT ![self] = G_val]
                                   /\ start_check_id' = [start_check_id EXCEPT ![self] = 0]
                                   /\ pc' = [pc EXCEPT ![self] = "restart_global"]
                                   /\ UNCHANGED << G_locked, 
                                                   help_update_result >>
                        /\ UNCHANGED << stack, ii, local_R_val >>
             /\ UNCHANGED << G_val, value, R_val, R_locked, read_result, 
                             write_result, History, old_val, new_val, iii, 
                             num_op >>

HP3(self) == /\ pc[self] = "HP3"
             /\ local_R_val' = [local_R_val EXCEPT ![self] = R_val[ii[self]]]
             /\ IF local_R_val'[self] /= expected_val[self]
                   THEN /\ cur_G_val' = [cur_G_val EXCEPT ![self] = G_val]
                        /\ IF cur_G_val'[self] /= expected_val[self]
                              THEN /\ expected_val' = [expected_val EXCEPT ![self] = cur_G_val'[self]]
                                   /\ start_check_id' = [start_check_id EXCEPT ![self] = 0]
                                   /\ pc' = [pc EXCEPT ![self] = "restart_global"]
                                   /\ UNCHANGED << R_val, R_locked >>
                              ELSE /\ R_val' = [R_val EXCEPT ![ii[self]] = expected_val[self]]
                                   /\ R_locked' = [R_locked EXCEPT ![ii[self]] = TRUE]
                                   /\ pc' = [pc EXCEPT ![self] = "HP4"]
                                   /\ UNCHANGED << expected_val, 
                                                   start_check_id >>
                   ELSE /\ pc' = [pc EXCEPT ![self] = "HP4"]
                        /\ UNCHANGED << R_val, R_locked, expected_val, 
                                        start_check_id, cur_G_val >>
             /\ UNCHANGED << G_val, G_locked, value, read_result, write_result, 
                             help_update_result, History, stack, ii, old_val, 
                             new_val, iii, num_op >>

HP4(self) == /\ pc[self] = "HP4"
             /\ ii' = [ii EXCEPT ![self] = ii[self] + 1]
             /\ pc' = [pc EXCEPT ![self] = "HP2"]
             /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                             read_result, write_result, help_update_result, 
                             History, stack, expected_val, start_check_id, 
                             cur_G_val, local_R_val, old_val, new_val, iii, 
                             num_op >>

HP5(self) == /\ pc[self] = "HP5"
             /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
             /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
             /\ cur_G_val' = [cur_G_val EXCEPT ![self] = Head(stack[self]).cur_G_val]
             /\ local_R_val' = [local_R_val EXCEPT ![self] = Head(stack[self]).local_R_val]
             /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
             /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
             /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
             /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                             read_result, write_result, help_update_result, 
                             History, old_val, new_val, iii, num_op >>

HelpUpdate(self) == restart_global(self) \/ HP0(self) \/ HP1(self)
                       \/ HP2(self) \/ HP3(self) \/ HP4(self) \/ HP5(self)

W0(self) == /\ pc[self] = "W0"
            /\ IF G_locked = TRUE
                  THEN /\ /\ expected_val' = [expected_val EXCEPT ![self] = G_val]
                          /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "HelpUpdate",
                                                                   pc        |->  "W1",
                                                                   ii        |->  ii[self],
                                                                   cur_G_val |->  cur_G_val[self],
                                                                   local_R_val |->  local_R_val[self],
                                                                   expected_val |->  expected_val[self],
                                                                   start_check_id |->  start_check_id[self] ] >>
                                                               \o stack[self]]
                          /\ start_check_id' = [start_check_id EXCEPT ![self] = 0]
                       /\ ii' = [ii EXCEPT ![self] = 0]
                       /\ cur_G_val' = [cur_G_val EXCEPT ![self] = 0]
                       /\ local_R_val' = [local_R_val EXCEPT ![self] = 0]
                       /\ pc' = [pc EXCEPT ![self] = "restart_global"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "W1"]
                       /\ UNCHANGED << stack, expected_val, start_check_id, ii, 
                                       cur_G_val, local_R_val >>
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, old_val, new_val, iii, num_op >>

W1(self) == /\ pc[self] = "W1"
            /\ IF G_val /= old_val[self]
                  THEN /\ write_result' = [write_result EXCEPT ![self] = FALSE]
                       /\ pc' = [pc EXCEPT ![self] = "W2"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "W3"]
                       /\ UNCHANGED write_result
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, help_update_result, History, stack, 
                            expected_val, start_check_id, ii, cur_G_val, 
                            local_R_val, old_val, new_val, iii, num_op >>

W2(self) == /\ pc[self] = "W2"
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ iii' = [iii EXCEPT ![self] = Head(stack[self]).iii]
            /\ old_val' = [old_val EXCEPT ![self] = Head(stack[self]).old_val]
            /\ new_val' = [new_val EXCEPT ![self] = Head(stack[self]).new_val]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, expected_val, start_check_id, ii, 
                            cur_G_val, local_R_val, num_op >>

W3(self) == /\ pc[self] = "W3"
            /\ G_locked' = TRUE
            /\ G_val' = new_val[self]
            /\ iii' = [iii EXCEPT ![self] = 0]
            /\ pc' = [pc EXCEPT ![self] = "W4"]
            /\ UNCHANGED << value, R_val, R_locked, read_result, write_result, 
                            help_update_result, History, stack, expected_val, 
                            start_check_id, ii, cur_G_val, local_R_val, 
                            old_val, new_val, num_op >>

W4(self) == /\ pc[self] = "W4"
            /\ IF iii[self] < NumThreads
                  THEN /\ pc' = [pc EXCEPT ![self] = "W5"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "W6"]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, stack, expected_val, start_check_id, ii, 
                            cur_G_val, local_R_val, old_val, new_val, iii, 
                            num_op >>

W5(self) == /\ pc[self] = "W5"
            /\ R_locked' = [R_locked EXCEPT ![iii[self]] = TRUE]
            /\ R_val' = [R_val EXCEPT ![iii[self]] = new_val[self]]
            /\ iii' = [iii EXCEPT ![self] = iii[self] + 1]
            /\ pc' = [pc EXCEPT ![self] = "W4"]
            /\ UNCHANGED << G_val, G_locked, value, read_result, write_result, 
                            help_update_result, History, stack, expected_val, 
                            start_check_id, ii, cur_G_val, local_R_val, 
                            old_val, new_val, num_op >>

W6(self) == /\ pc[self] = "W6"
            /\ write_result' = [write_result EXCEPT ![self] = TRUE]
            /\ History' = Append(History, [type |-> "write",
                                       data |-> new_val[self],
                                       thread |-> self])
            /\ pc' = [pc EXCEPT ![self] = "W7"]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, help_update_result, stack, 
                            expected_val, start_check_id, ii, cur_G_val, 
                            local_R_val, old_val, new_val, iii, num_op >>

W7(self) == /\ pc[self] = "W7"
            /\ G_locked' = FALSE
            /\ pc' = [pc EXCEPT ![self] = "W8"]
            /\ UNCHANGED << G_val, value, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, stack, 
                            expected_val, start_check_id, ii, cur_G_val, 
                            local_R_val, old_val, new_val, iii, num_op >>

W8(self) == /\ pc[self] = "W8"
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ iii' = [iii EXCEPT ![self] = Head(stack[self]).iii]
            /\ old_val' = [old_val EXCEPT ![self] = Head(stack[self]).old_val]
            /\ new_val' = [new_val EXCEPT ![self] = Head(stack[self]).new_val]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, expected_val, start_check_id, ii, 
                            cur_G_val, local_R_val, num_op >>

writer(self) == W0(self) \/ W1(self) \/ W2(self) \/ W3(self) \/ W4(self)
                   \/ W5(self) \/ W6(self) \/ W7(self) \/ W8(self)

R0(self) == /\ pc[self] = "R0"
            /\ IF R_locked[self] = FALSE
                  THEN /\ read_result' = [read_result EXCEPT ![self] = R_val[self]]
                       /\ History' = Append(History, [type |-> "read",
                                                  data |-> read_result'[self],
                                                  thread |-> self])
                       /\ pc' = [pc EXCEPT ![self] = "R3"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "R1"]
                       /\ UNCHANGED << read_result, History >>
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            write_result, help_update_result, stack, 
                            expected_val, start_check_id, ii, cur_G_val, 
                            local_R_val, old_val, new_val, iii, num_op >>

R1(self) == /\ pc[self] = "R1"
            /\ /\ expected_val' = [expected_val EXCEPT ![self] = R_val[self]]
               /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "HelpUpdate",
                                                        pc        |->  "R2",
                                                        ii        |->  ii[self],
                                                        cur_G_val |->  cur_G_val[self],
                                                        local_R_val |->  local_R_val[self],
                                                        expected_val |->  expected_val[self],
                                                        start_check_id |->  start_check_id[self] ] >>
                                                    \o stack[self]]
               /\ start_check_id' = [start_check_id EXCEPT ![self] = self + 1]
            /\ ii' = [ii EXCEPT ![self] = 0]
            /\ cur_G_val' = [cur_G_val EXCEPT ![self] = 0]
            /\ local_R_val' = [local_R_val EXCEPT ![self] = 0]
            /\ pc' = [pc EXCEPT ![self] = "restart_global"]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, old_val, new_val, iii, num_op >>

R2(self) == /\ pc[self] = "R2"
            /\ read_result' = [read_result EXCEPT ![self] = help_update_result[self]]
            /\ History' = Append(History, [type |-> "read",
                                       data |-> read_result'[self],
                                       thread |-> self])
            /\ pc' = [pc EXCEPT ![self] = "R3"]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            write_result, help_update_result, stack, 
                            expected_val, start_check_id, ii, cur_G_val, 
                            local_R_val, old_val, new_val, iii, num_op >>

R3(self) == /\ pc[self] = "R3"
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                            read_result, write_result, help_update_result, 
                            History, expected_val, start_check_id, ii, 
                            cur_G_val, local_R_val, old_val, new_val, iii, 
                            num_op >>

reader(self) == R0(self) \/ R1(self) \/ R2(self) \/ R3(self)

thread_loop(self) == /\ pc[self] = "thread_loop"
                     /\ IF num_op[self] < MaxNumOp
                           THEN /\ num_op' = [num_op EXCEPT ![self] = num_op[self] + 1]
                                /\ \/ /\ pc' = [pc EXCEPT ![self] = "read"]
                                   \/ /\ pc' = [pc EXCEPT ![self] = "write"]
                           ELSE /\ pc' = [pc EXCEPT ![self] = "Done"]
                                /\ UNCHANGED num_op
                     /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                                     read_result, write_result, 
                                     help_update_result, History, stack, 
                                     expected_val, start_check_id, ii, 
                                     cur_G_val, local_R_val, old_val, new_val, 
                                     iii >>

read(self) == /\ pc[self] = "read"
              /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "reader",
                                                       pc        |->  "thread_loop" ] >>
                                                   \o stack[self]]
              /\ pc' = [pc EXCEPT ![self] = "R0"]
              /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                              read_result, write_result, help_update_result, 
                              History, expected_val, start_check_id, ii, 
                              cur_G_val, local_R_val, old_val, new_val, iii, 
                              num_op >>

write(self) == /\ pc[self] = "write"
               /\ /\ new_val' = [new_val EXCEPT ![self] = G_val + 1]
                  /\ old_val' = [old_val EXCEPT ![self] = G_val]
                  /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "writer",
                                                           pc        |->  "thread_loop",
                                                           iii       |->  iii[self],
                                                           old_val   |->  old_val[self],
                                                           new_val   |->  new_val[self] ] >>
                                                       \o stack[self]]
               /\ iii' = [iii EXCEPT ![self] = 0]
               /\ pc' = [pc EXCEPT ![self] = "W0"]
               /\ UNCHANGED << G_val, G_locked, value, R_val, R_locked, 
                               read_result, write_result, help_update_result, 
                               History, expected_val, start_check_id, ii, 
                               cur_G_val, local_R_val, num_op >>

thread_id(self) == thread_loop(self) \/ read(self) \/ write(self)

(* Allow infinite stuttering to prevent deadlock on termination. *)
Terminating == /\ \A self \in ProcSet: pc[self] = "Done"
               /\ UNCHANGED vars

Next == (\E self \in ProcSet:  \/ HelpUpdate(self) \/ writer(self)
                               \/ reader(self))
           \/ (\E self \in Threads: thread_id(self))
           \/ Terminating

Spec == /\ Init /\ [][Next]_vars
        /\ \A self \in Threads : /\ WF_vars(thread_id(self))
                                 /\ WF_vars(reader(self))
                                 /\ WF_vars(writer(self))
                                 /\ WF_vars(HelpUpdate(self))

Termination == <>(\A self \in ProcSet: pc[self] = "Done")

\* END TRANSLATION

-----------------------------------------------------------------------------

(* enable these invariants in model checker *)

(* Check elements in History are type of Opertion *)
TypeOK == {History[i] : i \in DOMAIN History} \subseteq Operations

(* Operation in history h is monitonic *)
Monotonic(h) == \A i, j \in DOMAIN h : i <= j => h[i].data <= h[j].data

ReadYourWrite == \A i, j \in DOMAIN History : /\ i < j
                                              /\ History[i].type = "write"
                                              /\ History[j].type = "read"
                                              /\ History[i].thread = History[j].thread
                                              => History[j].data >= History[i].data

(* Read the latest writes *)
ReadAfterWrite == \A i, j \in DOMAIN History : /\ i < j
                                               /\ History[i].type = "write"
                                               /\ History[j].type = "read"
                                               => History[j].data >= History[i].data

Linearizability == \A i, j \in DOMAIN History : /\ i < j
                                                => History[j].data >= History[i].data

Invariant == /\ Linearizability 
          /\ Monotonic(History)
          /\ ReadAfterWrite 
          /\ TypeOK

=============================================================================

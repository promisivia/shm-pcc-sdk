-------------------------- MODULE help_update --------------------------
EXTENDS Naturals, Integers, Reals, Sequences, FiniteSets, TLC

CONSTANT NumThreads  (* 副本指针的数量 (NumThreads > 0) *)
ASSUME NumThreads \in Nat /\ NumThreads > 0

CONSTANT MaxNumOp

Operations == [type: {"write"}, data: Nat, thread: 0..NumThreads-1, start_time: Nat, end_time: Nat]
       \union [type: {"read"}, data: Nat, thread: 0..NumThreads-1, start_time: Nat, end_time: Nat]

Threads == 0..NumThreads-1

(*
--algorithm HelpUpdateMechanism
{
variables
(* Global pointer value (simulates clear_lock_bit(global_ptr))*)
G_val = 0;              
(* Global pointer lock bit (simulates has_lock_bit(global_ptr))*)
G_locked = FALSE;
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
(* Global Logical Time *)
LogicTime = 0;

(* HelpUpdate procedure *)
procedure HelpUpdate(expected_val, start_check_id) 
variable ii = 0; \* 局部变量：ii, cur_G_val, local_R_val
{
HP0:  
    (* 依次帮助更新每个 Replica *)
    ii := start_check_id;
HP1:
    while (ii < NumThreads) {
        (* 检查全局状态，如果已完成或改变，则退出/重启 *)
HP2:
        if (G_val = expected_val /\ G_locked = FALSE) {
            help_update_result[self] := TRUE; \* 成功返回
            return;
        };
        
        (* 然后尝试依次更新每个 replica *)
HP3:
        (* 可以假设有人在一直更新，可以先check是不是已经更新成功了 *)
        (* 注意：在 PlusCal 中，replica 的 lock bit 始终为 FALSE，因为它不承担 Writer 的 commit 锁。 *)
        if (R_val[ii] /= expected_val) {
            (* 需要帮助更新，但必须先判断是不是因为 global 变了 *)
HP4:
            if (G_val /= expected_val) {
                (* 说明有其他人成功更新G_val，clear lock，然后又成功更新了新的G_val，立刻返回失败 *)
                help_update_result[self] := FALSE;
                return;
            } else {
                (* 不是因为 global 变了，说明这个 replica 更旧，需要帮助更新 *)
                R_val[ii] := expected_val;
                R_locked[ii] := TRUE; \* 模拟 set_lock_bit(expected_val)
            }
        };
HP6:
        ii := ii + 1;
    };
    
HP7:
    (* 这一轮检查，所有的 replica 和 global 的值都更新为 expected_val (带锁位) 了 *)
    if (G_val = expected_val /\ G_locked = TRUE) {
        (* 尝试 CAS G 从 locked 到 unlocked (current_cas(global_ptr, cur_global_ptr, expected_val)) *)
        (* Here is the LP of reader/writer !!! *)
        \* call markLP();
        G_locked := FALSE; \* CAS 成功 (清除锁位)
        help_update_result[self] := TRUE; \* 成功返回
HP10:
        return;
    } else {
        (* G_val 改变了 *)
        help_update_result[self] := FALSE;
        return;
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
    } else {
        (* 2: CAS 成功 (Commit Point: 锁定) *)
        G_locked := TRUE; 
        G_val := new_val;
    };

W3:
    call HelpUpdate(new_val, 0);
W4:
    write_result[self] := TRUE;
    return;
}

procedure reader()
variable local_R_val = 0;
{
R0: (* 1: 如果R[i]没有锁位，直接读取R[i]的值 *)
    local_R_val := R_val[self];
    (* Case (1): 如果R[i]以.0结尾，说明当前的R[i]不是更新的G，可以直接从R[i]开始 *)
    if (R_locked[self] = FALSE) {
        read_result[self] := local_R_val;
    } else {
R1: (* 2: 如果R[i]有锁位，先帮助更新R[i]的值 *)
        (* 这里的commit point应该在writer的commit point之后，下一次修改之前 *)
        call HelpUpdate(local_R_val, self + 1); (* HelpUpdate 返回最终 clean value G_val *)
R3:
        if (help_update_result[self] = TRUE) {
            R_locked[self] := FALSE;
        };
        read_result[self] := local_R_val;
    };
R4:
    return;
}

(* -------------------------------------------------------------- *)
(* --------------------- THREAD ACTIONS ------------------------- *)
(* -------------------------------------------------------------- *)
fair process (thread_id \in Threads)
variables _num_op = 0, _start_time = 0, _end_time = 0, value = 0;
{
    thread_loop:
    while (_num_op < MaxNumOp) {
        _num_op := _num_op + 1;
        either {
        read:
            LogicTime := LogicTime + 1;
            _start_time := LogicTime;
            call reader();
P0:
            LogicTime := LogicTime + 1;
            _end_time := LogicTime;
            History := Append(History, [type |-> "read", data |-> read_result[self], thread |-> self, start_time |-> _start_time, end_time |-> _end_time]);
        } or {
        write:
            LogicTime := LogicTime + 1;
            _start_time := LogicTime;
            value := G_val;

            call writer(value, value + 1);
P1:
            LogicTime := LogicTime + 1;
            _end_time := LogicTime;
            if (write_result[self] = TRUE) {
                History := Append(History, [type |-> "write", data |-> value + 1, thread |-> self, start_time |-> _start_time, end_time |-> _end_time]);
            };
        }
    }
}
}*) (* END of algorithm *)

\* BEGIN TRANSLATION
CONSTANT defaultInitValue
VARIABLES G_val, G_locked, R_val, R_locked, read_result, write_result, 
          help_update_result, History, LogicTime, pc, stack, expected_val, 
          start_check_id, ii, old_val, new_val, iii, local_R_val, _num_op, 
          _start_time, _end_time, value

vars == << G_val, G_locked, R_val, R_locked, read_result, write_result, 
           help_update_result, History, LogicTime, pc, stack, expected_val, 
           start_check_id, ii, old_val, new_val, iii, local_R_val, _num_op, 
           _start_time, _end_time, value >>

ProcSet == (Threads)

Init == (* Global variables *)
        /\ G_val = 0
        /\ G_locked = FALSE
        /\ R_val = [i \in Threads |-> 0]
        /\ R_locked = [i \in Threads |-> FALSE]
        /\ read_result = [i \in Threads |-> 0]
        /\ write_result = [i \in Threads |-> FALSE]
        /\ help_update_result = [i \in Threads |-> 0]
        /\ History = <<>>
        /\ LogicTime = 0
        (* Procedure HelpUpdate *)
        /\ expected_val = [ self \in ProcSet |-> defaultInitValue]
        /\ start_check_id = [ self \in ProcSet |-> defaultInitValue]
        /\ ii = [ self \in ProcSet |-> 0]
        (* Procedure writer *)
        /\ old_val = [ self \in ProcSet |-> defaultInitValue]
        /\ new_val = [ self \in ProcSet |-> defaultInitValue]
        /\ iii = [ self \in ProcSet |-> 0]
        (* Procedure reader *)
        /\ local_R_val = [ self \in ProcSet |-> 0]
        (* Process thread_id *)
        /\ _num_op = [self \in Threads |-> 0]
        /\ _start_time = [self \in Threads |-> 0]
        /\ _end_time = [self \in Threads |-> 0]
        /\ value = [self \in Threads |-> 0]
        /\ stack = [self \in ProcSet |-> << >>]
        /\ pc = [self \in ProcSet |-> "thread_loop"]

HP0(self) == /\ pc[self] = "HP0"
             /\ ii' = [ii EXCEPT ![self] = start_check_id[self]]
             /\ pc' = [pc EXCEPT ![self] = "HP1"]
             /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                             write_result, help_update_result, History, 
                             LogicTime, stack, expected_val, start_check_id, 
                             old_val, new_val, iii, local_R_val, _num_op, 
                             _start_time, _end_time, value >>

HP1(self) == /\ pc[self] = "HP1"
             /\ IF ii[self] < NumThreads
                   THEN /\ pc' = [pc EXCEPT ![self] = "HP2"]
                   ELSE /\ pc' = [pc EXCEPT ![self] = "HP7"]
             /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                             write_result, help_update_result, History, 
                             LogicTime, stack, expected_val, start_check_id, 
                             ii, old_val, new_val, iii, local_R_val, _num_op, 
                             _start_time, _end_time, value >>

HP2(self) == /\ pc[self] = "HP2"
             /\ IF G_val = expected_val[self] /\ G_locked = FALSE
                   THEN /\ help_update_result' = [help_update_result EXCEPT ![self] = TRUE]
                        /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
                        /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
                        /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
                        /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
                        /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
                   ELSE /\ pc' = [pc EXCEPT ![self] = "HP3"]
                        /\ UNCHANGED << help_update_result, stack, 
                                        expected_val, start_check_id, ii >>
             /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                             write_result, History, LogicTime, old_val, 
                             new_val, iii, local_R_val, _num_op, _start_time, 
                             _end_time, value >>

HP3(self) == /\ pc[self] = "HP3"
             /\ IF R_val[ii[self]] /= expected_val[self]
                   THEN /\ pc' = [pc EXCEPT ![self] = "HP4"]
                   ELSE /\ pc' = [pc EXCEPT ![self] = "HP6"]
             /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                             write_result, help_update_result, History, 
                             LogicTime, stack, expected_val, start_check_id, 
                             ii, old_val, new_val, iii, local_R_val, _num_op, 
                             _start_time, _end_time, value >>

HP4(self) == /\ pc[self] = "HP4"
             /\ IF G_val /= expected_val[self]
                   THEN /\ help_update_result' = [help_update_result EXCEPT ![self] = FALSE]
                        /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
                        /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
                        /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
                        /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
                        /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
                        /\ UNCHANGED << R_val, R_locked >>
                   ELSE /\ R_val' = [R_val EXCEPT ![ii[self]] = expected_val[self]]
                        /\ R_locked' = [R_locked EXCEPT ![ii[self]] = TRUE]
                        /\ pc' = [pc EXCEPT ![self] = "HP6"]
                        /\ UNCHANGED << help_update_result, stack, 
                                        expected_val, start_check_id, ii >>
             /\ UNCHANGED << G_val, G_locked, read_result, write_result, 
                             History, LogicTime, old_val, new_val, iii, 
                             local_R_val, _num_op, _start_time, _end_time, 
                             value >>

HP6(self) == /\ pc[self] = "HP6"
             /\ ii' = [ii EXCEPT ![self] = ii[self] + 1]
             /\ pc' = [pc EXCEPT ![self] = "HP1"]
             /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                             write_result, help_update_result, History, 
                             LogicTime, stack, expected_val, start_check_id, 
                             old_val, new_val, iii, local_R_val, _num_op, 
                             _start_time, _end_time, value >>

HP7(self) == /\ pc[self] = "HP7"
             /\ IF G_val = expected_val[self] /\ G_locked = TRUE
                   THEN /\ G_locked' = FALSE
                        /\ help_update_result' = [help_update_result EXCEPT ![self] = TRUE]
                        /\ pc' = [pc EXCEPT ![self] = "HP10"]
                        /\ UNCHANGED << stack, expected_val, start_check_id, 
                                        ii >>
                   ELSE /\ help_update_result' = [help_update_result EXCEPT ![self] = FALSE]
                        /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
                        /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
                        /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
                        /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
                        /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
                        /\ UNCHANGED G_locked
             /\ UNCHANGED << G_val, R_val, R_locked, read_result, write_result, 
                             History, LogicTime, old_val, new_val, iii, 
                             local_R_val, _num_op, _start_time, _end_time, 
                             value >>

HP10(self) == /\ pc[self] = "HP10"
              /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
              /\ ii' = [ii EXCEPT ![self] = Head(stack[self]).ii]
              /\ expected_val' = [expected_val EXCEPT ![self] = Head(stack[self]).expected_val]
              /\ start_check_id' = [start_check_id EXCEPT ![self] = Head(stack[self]).start_check_id]
              /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
              /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                              write_result, help_update_result, History, 
                              LogicTime, old_val, new_val, iii, local_R_val, 
                              _num_op, _start_time, _end_time, value >>

HelpUpdate(self) == HP0(self) \/ HP1(self) \/ HP2(self) \/ HP3(self)
                       \/ HP4(self) \/ HP6(self) \/ HP7(self) \/ HP10(self)

W0(self) == /\ pc[self] = "W0"
            /\ IF G_locked = TRUE
                  THEN /\ /\ expected_val' = [expected_val EXCEPT ![self] = G_val]
                          /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "HelpUpdate",
                                                                   pc        |->  "W1",
                                                                   ii        |->  ii[self],
                                                                   expected_val |->  expected_val[self],
                                                                   start_check_id |->  start_check_id[self] ] >>
                                                               \o stack[self]]
                          /\ start_check_id' = [start_check_id EXCEPT ![self] = 0]
                       /\ ii' = [ii EXCEPT ![self] = 0]
                       /\ pc' = [pc EXCEPT ![self] = "HP0"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "W1"]
                       /\ UNCHANGED << stack, expected_val, start_check_id, ii >>
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, 
                            LogicTime, old_val, new_val, iii, local_R_val, 
                            _num_op, _start_time, _end_time, value >>

W1(self) == /\ pc[self] = "W1"
            /\ IF G_val /= old_val[self]
                  THEN /\ write_result' = [write_result EXCEPT ![self] = FALSE]
                       /\ pc' = [pc EXCEPT ![self] = "W2"]
                       /\ UNCHANGED << G_val, G_locked >>
                  ELSE /\ G_locked' = TRUE
                       /\ G_val' = new_val[self]
                       /\ pc' = [pc EXCEPT ![self] = "W3"]
                       /\ UNCHANGED write_result
            /\ UNCHANGED << R_val, R_locked, read_result, help_update_result, 
                            History, LogicTime, stack, expected_val, 
                            start_check_id, ii, old_val, new_val, iii, 
                            local_R_val, _num_op, _start_time, _end_time, 
                            value >>

W2(self) == /\ pc[self] = "W2"
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ iii' = [iii EXCEPT ![self] = Head(stack[self]).iii]
            /\ old_val' = [old_val EXCEPT ![self] = Head(stack[self]).old_val]
            /\ new_val' = [new_val EXCEPT ![self] = Head(stack[self]).new_val]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, 
                            LogicTime, expected_val, start_check_id, ii, 
                            local_R_val, _num_op, _start_time, _end_time, 
                            value >>

W3(self) == /\ pc[self] = "W3"
            /\ /\ expected_val' = [expected_val EXCEPT ![self] = new_val[self]]
               /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "HelpUpdate",
                                                        pc        |->  "W4",
                                                        ii        |->  ii[self],
                                                        expected_val |->  expected_val[self],
                                                        start_check_id |->  start_check_id[self] ] >>
                                                    \o stack[self]]
               /\ start_check_id' = [start_check_id EXCEPT ![self] = 0]
            /\ ii' = [ii EXCEPT ![self] = 0]
            /\ pc' = [pc EXCEPT ![self] = "HP0"]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, 
                            LogicTime, old_val, new_val, iii, local_R_val, 
                            _num_op, _start_time, _end_time, value >>

W4(self) == /\ pc[self] = "W4"
            /\ write_result' = [write_result EXCEPT ![self] = TRUE]
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ iii' = [iii EXCEPT ![self] = Head(stack[self]).iii]
            /\ old_val' = [old_val EXCEPT ![self] = Head(stack[self]).old_val]
            /\ new_val' = [new_val EXCEPT ![self] = Head(stack[self]).new_val]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            help_update_result, History, LogicTime, 
                            expected_val, start_check_id, ii, local_R_val, 
                            _num_op, _start_time, _end_time, value >>

writer(self) == W0(self) \/ W1(self) \/ W2(self) \/ W3(self) \/ W4(self)

R0(self) == /\ pc[self] = "R0"
            /\ local_R_val' = [local_R_val EXCEPT ![self] = R_val[self]]
            /\ IF R_locked[self] = FALSE
                  THEN /\ read_result' = [read_result EXCEPT ![self] = local_R_val'[self]]
                       /\ pc' = [pc EXCEPT ![self] = "R4"]
                  ELSE /\ pc' = [pc EXCEPT ![self] = "R1"]
                       /\ UNCHANGED read_result
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, write_result, 
                            help_update_result, History, LogicTime, stack, 
                            expected_val, start_check_id, ii, old_val, new_val, 
                            iii, _num_op, _start_time, _end_time, value >>

R1(self) == /\ pc[self] = "R1"
            /\ /\ expected_val' = [expected_val EXCEPT ![self] = local_R_val[self]]
               /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "HelpUpdate",
                                                        pc        |->  "R3",
                                                        ii        |->  ii[self],
                                                        expected_val |->  expected_val[self],
                                                        start_check_id |->  start_check_id[self] ] >>
                                                    \o stack[self]]
               /\ start_check_id' = [start_check_id EXCEPT ![self] = self + 1]
            /\ ii' = [ii EXCEPT ![self] = 0]
            /\ pc' = [pc EXCEPT ![self] = "HP0"]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, 
                            LogicTime, old_val, new_val, iii, local_R_val, 
                            _num_op, _start_time, _end_time, value >>

R3(self) == /\ pc[self] = "R3"
            /\ IF help_update_result[self] = TRUE
                  THEN /\ R_locked' = [R_locked EXCEPT ![self] = FALSE]
                  ELSE /\ TRUE
                       /\ UNCHANGED R_locked
            /\ read_result' = [read_result EXCEPT ![self] = local_R_val[self]]
            /\ pc' = [pc EXCEPT ![self] = "R4"]
            /\ UNCHANGED << G_val, G_locked, R_val, write_result, 
                            help_update_result, History, LogicTime, stack, 
                            expected_val, start_check_id, ii, old_val, new_val, 
                            iii, local_R_val, _num_op, _start_time, _end_time, 
                            value >>

R4(self) == /\ pc[self] = "R4"
            /\ pc' = [pc EXCEPT ![self] = Head(stack[self]).pc]
            /\ local_R_val' = [local_R_val EXCEPT ![self] = Head(stack[self]).local_R_val]
            /\ stack' = [stack EXCEPT ![self] = Tail(stack[self])]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, History, 
                            LogicTime, expected_val, start_check_id, ii, 
                            old_val, new_val, iii, _num_op, _start_time, 
                            _end_time, value >>

reader(self) == R0(self) \/ R1(self) \/ R3(self) \/ R4(self)

thread_loop(self) == /\ pc[self] = "thread_loop"
                     /\ IF _num_op[self] < MaxNumOp
                           THEN /\ _num_op' = [_num_op EXCEPT ![self] = _num_op[self] + 1]
                                /\ \/ /\ pc' = [pc EXCEPT ![self] = "read"]
                                   \/ /\ pc' = [pc EXCEPT ![self] = "write"]
                           ELSE /\ pc' = [pc EXCEPT ![self] = "Done"]
                                /\ UNCHANGED _num_op
                     /\ UNCHANGED << G_val, G_locked, R_val, R_locked, 
                                     read_result, write_result, 
                                     help_update_result, History, LogicTime, 
                                     stack, expected_val, start_check_id, ii, 
                                     old_val, new_val, iii, local_R_val, 
                                     _start_time, _end_time, value >>

read(self) == /\ pc[self] = "read"
              /\ LogicTime' = LogicTime + 1
              /\ _start_time' = [_start_time EXCEPT ![self] = LogicTime']
              /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "reader",
                                                       pc        |->  "P0",
                                                       local_R_val |->  local_R_val[self] ] >>
                                                   \o stack[self]]
              /\ local_R_val' = [local_R_val EXCEPT ![self] = 0]
              /\ pc' = [pc EXCEPT ![self] = "R0"]
              /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                              write_result, help_update_result, History, 
                              expected_val, start_check_id, ii, old_val, 
                              new_val, iii, _num_op, _end_time, value >>

P0(self) == /\ pc[self] = "P0"
            /\ LogicTime' = LogicTime + 1
            /\ _end_time' = [_end_time EXCEPT ![self] = LogicTime']
            /\ History' = Append(History, [type |-> "read", data |-> read_result[self], thread |-> self, start_time |-> _start_time[self], end_time |-> _end_time'[self]])
            /\ pc' = [pc EXCEPT ![self] = "thread_loop"]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, stack, 
                            expected_val, start_check_id, ii, old_val, new_val, 
                            iii, local_R_val, _num_op, _start_time, value >>

write(self) == /\ pc[self] = "write"
               /\ LogicTime' = LogicTime + 1
               /\ _start_time' = [_start_time EXCEPT ![self] = LogicTime']
               /\ value' = [value EXCEPT ![self] = G_val]
               /\ /\ new_val' = [new_val EXCEPT ![self] = value'[self] + 1]
                  /\ old_val' = [old_val EXCEPT ![self] = value'[self]]
                  /\ stack' = [stack EXCEPT ![self] = << [ procedure |->  "writer",
                                                           pc        |->  "P1",
                                                           iii       |->  iii[self],
                                                           old_val   |->  old_val[self],
                                                           new_val   |->  new_val[self] ] >>
                                                       \o stack[self]]
               /\ iii' = [iii EXCEPT ![self] = 0]
               /\ pc' = [pc EXCEPT ![self] = "W0"]
               /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                               write_result, help_update_result, History, 
                               expected_val, start_check_id, ii, local_R_val, 
                               _num_op, _end_time >>

P1(self) == /\ pc[self] = "P1"
            /\ LogicTime' = LogicTime + 1
            /\ _end_time' = [_end_time EXCEPT ![self] = LogicTime']
            /\ IF write_result[self] = TRUE
                  THEN /\ History' = Append(History, [type |-> "write", data |-> value[self] + 1, thread |-> self, start_time |-> _start_time[self], end_time |-> _end_time'[self]])
                  ELSE /\ TRUE
                       /\ UNCHANGED History
            /\ pc' = [pc EXCEPT ![self] = "thread_loop"]
            /\ UNCHANGED << G_val, G_locked, R_val, R_locked, read_result, 
                            write_result, help_update_result, stack, 
                            expected_val, start_check_id, ii, old_val, new_val, 
                            iii, local_R_val, _num_op, _start_time, value >>

thread_id(self) == thread_loop(self) \/ read(self) \/ P0(self)
                      \/ write(self) \/ P1(self)

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

\* Linearizability == \A i, j \in DOMAIN History : /\ i < j
\*                                                 => History[j].data >= History[i].data

Linearizability == \A i, j \in DOMAIN History : 
    /\ i < j 
    /\ History[i].end_time <= History[j].start_time
    => History[j].data < History[i].data

GlobalAndReplicas == G_locked = FALSE => \E j \in Threads : R_val[j] = G_val

Invariant == /\ Linearizability 
        \*   /\ Monotonic(History)ß
        \*   /\ ReadAfterWrite 
          /\ TypeOK
          /\ GlobalAndReplicas

=============================================================================

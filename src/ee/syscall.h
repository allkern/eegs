static const char* ee_get_syscall(int n) {
    switch (n) {
        case 0x01: return "void ResetEE(int reset_flag)";
        case 0x02: return "void SetGsCrt(bool interlaced, int display_mode, bool frame)";
        case 0x04: return "void Exit(int status)";
        case 0x05: return "void _ExceptionEpilogue()";
        case 0x06: return "void LoadExecPS2(const char* filename, int argc, char** argv)";
        case 0x07: return "void ExecPS2(void* entry, void* gp, int argc, char** argv)";
        case 0x10: return "int AddIntcHandler(int int_cause, int (*handler)(int), int next, void* arg, int flag)";
        case 0x11: return "int RemoveIntcHandler(int int_cause, int handler_id)";
        case 0x12: return "int AddDmacHandler(int dma_cause, int (*handler)(int), int next, void* arg, int flag)";
        case 0x13: return "int RemoveDmacHandler(int dma_cause, int handler_id)";
        case 0x14: return "bool _EnableIntc(int cause_bit)";
        case 0x15: return "bool _DisableIntc(int cause_bit)";
        case 0x16: return "bool _EnableDmac(int cause_bit)";
        case 0x17: return "bool _DisableDmac(int cause_bit)";
        case 0x20: return "int CreateThread(ThreadParam* t)";
        case 0x21: return "void DeleteThread(int thread_id)";
        case 0x22: return "void StartThread(int thread_id, void* arg)";
        case 0x23: return "void ExitThread()";
        case 0x24: return "void ExitDeleteThread()";
        case 0x25: return "void TerminateThread(int thread_id)";
        case 0x26: return "void iTerminateThread(int thread_id)";
        case 0x29: return "int ChangeThreadPriority(int thread_id, int priority)";
        case 0x2A: return "int iChangeThreadPriority(int thread_id, int priority)";
        case 0x2B: return "void RotateThreadReadyQueue(int priority)";
        case 0x2C: return "int _iRotateThreadReadyQueue(int priority)";
        case 0x2D: return "void ReleaseWaitThread(int thread_id)";
        case 0x2E: return "int iReleaseWaitThread(int thread_id)";
        case 0x2F: return "int GetThreadId()";
        case 0x30: return "int ReferThreadStatus(int thread_id, ThreadParam* status)";
        case 0x31: return "int iReferThreadStatus(int thread_id, ThreadParam* status)";
        case 0x32: return "void SleepThread()";
        case 0x33: return "void WakeupThread(int thread_id)";
        case 0x34: return "int iWakeupThread(int thread_id)";
        case 0x35: return "int CancelWakeupThread(int thread_id)";
        case 0x36: return "int iCancelWakeupThread(int thread_id)";
        case 0x37: return "int SuspendThread(int thread_id)";
        case 0x38: return "int iSuspendThread(int thread_id)";
        case 0x39: return "void ResumeThread(int thread_id)";
        case 0x3A: return "int iResumeThread(int thread_id)";
        case 0x3B: return "void JoinThread()";
        case 0x3C: return "void* InitMainThread(uint32 gp, void* stack, int stack_size, char* args, int root)";
        case 0x3D: return "void* InitHeap(void* heap, int heap_size)";
        case 0x3E: return "void* EndOfHeap()";
        case 0x40: return "int CreateSema(SemaParam* s)";
        case 0x41: return "int DeleteSema(int sema_id)";
        case 0x42: return "int SignalSema(int sema_id)";
        case 0x43: return "int iSignalSema(int sema_id)";
        case 0x44: return "void WaitSema(int sema_id)";
        case 0x45: return "int PollSema(int sema_id)";
        case 0x46: return "int iPollSema(int sema_id)";
        case 0x64: return "void FlushCache(int mode)";
        case 0x70: return "uint64_t GsGetIMR()";
        case 0x71: return "void GsPutIMR(uint64_t value)";
        case 0x73: return "void SetVSyncFlag(int* vsync_occurred, u64* csr_stat_on_vsync)";
        case 0x74: return "void SetSyscall(int index, int address)";
        case 0x76: return "int SifDmaStat(unsigned int dma_id)";
        case 0x77: return "unsigned int SifSetDma(SifDmaTransfer* trans, int len)";
        case 0x78: return "void SifSetDChain()";
        case 0x7B: return "void ExecOSD(int argc, char** argv)";
        case 0x7D: return "void PSMode()";
        case 0x7E: return "int MachineType()";
        case 0x7F: return "int GetMemorySize()";
    }

    return "<unknown>";
}
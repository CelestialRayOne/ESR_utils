using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace ESR_utils_App
{
    public static class Injector
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint access, bool inherit, int pid);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr VirtualAllocEx(IntPtr hProc, IntPtr addr, uint size, uint allocType, uint protect);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteProcessMemory(IntPtr hProc, IntPtr addr, byte[] buffer, uint size, out UIntPtr written);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateRemoteThread(IntPtr hProc, IntPtr attrs, uint stackSize, IntPtr startAddr, IntPtr param, uint flags, out uint tid);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetModuleHandle(string name);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string name);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr handle, uint ms);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr handle);

        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hReservedNull, uint dwFlags);

        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        private static extern bool FreeLibrary(IntPtr hModule);

        private const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
        private const uint MEM_COMMIT = 0x1000;
        private const uint MEM_RESERVE = 0x2000;
        private const uint PAGE_READWRITE = 0x04;

        public static bool Inject(int pid, string dllPath, out string error)
        {
            error = "";
            if (!File.Exists(dllPath))
            {
                error = "DLL not found: " + dllPath;
                return false;
            }

            var hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            if (hProc == IntPtr.Zero) { error = "OpenProcess failed: " + Marshal.GetLastWin32Error(); return false; }

            try
            {
                var pathBytes = System.Text.Encoding.ASCII.GetBytes(dllPath + "\0");
                var remote = VirtualAllocEx(hProc, IntPtr.Zero, (uint)pathBytes.Length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (remote == IntPtr.Zero) { error = "VirtualAllocEx failed"; return false; }

                if (!WriteProcessMemory(hProc, remote, pathBytes, (uint)pathBytes.Length, out _))
                { error = "WriteProcessMemory failed"; return false; }

                var loadLib = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
                if (loadLib == IntPtr.Zero) { error = "LoadLibraryA lookup failed"; return false; }

                var hThread = CreateRemoteThread(hProc, IntPtr.Zero, 0, loadLib, remote, 0, out _);
                if (hThread == IntPtr.Zero) { error = "CreateRemoteThread failed"; return false; }

                WaitForSingleObject(hThread, 10000);
                CloseHandle(hThread);
                return true;
            }
            finally
            {
                CloseHandle(hProc);
            }
        }

        public static bool UnloadDll(int pid, out string error)
        {
            error = "";
            var hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            if (hProc == IntPtr.Zero) { error = "OpenProcess failed"; return false; }

            try
            {
                IntPtr hMod = GetRemoteModuleHandle(pid, "ESR-utils.dll");
                if (hMod == IntPtr.Zero) { error = "DLL not loaded"; return false; }

                IntPtr pUnload = GetRemoteProcAddress(hMod, "UnloadDll", pid);
                if (pUnload == IntPtr.Zero) { error = "UnloadDll export not found"; return false; }

                IntPtr hThread = CreateRemoteThread(hProc, IntPtr.Zero, 0, pUnload, IntPtr.Zero, 0, out _);
                if (hThread == IntPtr.Zero) { error = "CreateRemoteThread failed"; return false; }

                WaitForSingleObject(hThread, 5000);
                CloseHandle(hThread);
                return true;
            }
            finally
            {
                CloseHandle(hProc);
            }
        }

        private static IntPtr GetRemoteModuleHandle(int pid, string moduleName)
        {
            var proc = System.Diagnostics.Process.GetProcessById(pid);
            foreach (System.Diagnostics.ProcessModule m in proc.Modules)
            {
                if (string.Equals(m.ModuleName, moduleName, StringComparison.OrdinalIgnoreCase))
                    return m.BaseAddress;
            }
            return IntPtr.Zero;
        }

        private static IntPtr GetRemoteProcAddress(IntPtr hModuleRemote, string funcName, int pid)
        {
            // Shortcut: kernel32's LoadLibraryA works across processes because kernel32 is at same address.
            // Same logic: if DLL was loaded from the same path locally, the export offset is the same.
            // So we: LoadLibrary locally, get local proc address, compute offset, apply to remote base.
            var dllPath = System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "ESR-utils.dll");
            IntPtr hLocal = LoadLibraryEx(dllPath, IntPtr.Zero, 0x00000001 /* DONT_RESOLVE_DLL_REFERENCES */);
            if (hLocal == IntPtr.Zero) return IntPtr.Zero;
            try
            {
                IntPtr pLocal = GetProcAddress(hLocal, funcName);
                if (pLocal == IntPtr.Zero) return IntPtr.Zero;
                long offset = pLocal.ToInt64() - hLocal.ToInt64();
                return new IntPtr(hModuleRemote.ToInt64() + offset);
            }
            finally
            {
                FreeLibrary(hLocal);
            }
        }
        public static bool IsDllLoaded(Process proc, string dllName)
        {
            try
            {
                foreach (ProcessModule mod in proc.Modules)
                {
                    if (string.Equals(mod.ModuleName, dllName, StringComparison.OrdinalIgnoreCase))
                        return true;
                }
            }
            catch { }
            return false;
        }
    }
}
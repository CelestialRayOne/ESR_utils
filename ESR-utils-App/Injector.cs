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
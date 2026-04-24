using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace ESR_utils_App
{
    public class D2RWatcher
    {
        public event Action<string> StatusChanged;

        private CancellationTokenSource _cts;
        private int _lastInjectedPid = -1;
        private bool _firstScan = true;

        public void Start()
        {
            _cts = new CancellationTokenSource();
            Task.Run(() => Loop(_cts.Token));
        }

        public void Stop()
        {
            _cts?.Cancel();
        }

        private void Loop(CancellationToken ct)
        {
            var dllPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "ESR-utils.dll");
            StatusChanged?.Invoke("Waiting for D2R...");

            while (!ct.IsCancellationRequested)
            {
                try
                {
                    var procs = Process.GetProcessesByName("D2R");
                    if (procs.Length == 0)
                    {
                        if (_lastInjectedPid != -1)
                        {
                            _lastInjectedPid = -1;
                            StatusChanged?.Invoke("Waiting for D2R...");
                        }
                        _firstScan = false;
                    }
                    else
                    {
                        var proc = procs[0];
                        if (proc.Id != _lastInjectedPid)
                        {
                            if (Injector.IsDllLoaded(proc, "ESR-utils.dll"))
                            {
                                _lastInjectedPid = proc.Id;
                                StatusChanged?.Invoke("Attached to D2R (pid " + proc.Id + ")");
                            }
                            else
                            {
                                bool d2rWasAlreadyRunning = _firstScan;
                                _firstScan = false;

                                try { proc.Refresh(); } catch { }
                                if (proc.MainWindowHandle == IntPtr.Zero)
                                {
                                    StatusChanged?.Invoke("Waiting for D2R window...");
                                    Thread.Sleep(2000);
                                    continue;
                                }

                                try { proc.WaitForInputIdle(30000); } catch { }

                                if (!d2rWasAlreadyRunning)
                                {
                                    StatusChanged?.Invoke("Waiting for D2R to stabilize...");
                                    Thread.Sleep(10000);
                                }

                                StatusChanged?.Invoke("Injecting into D2R...");
                                if (Injector.Inject(proc.Id, dllPath, out var err))
                                {
                                    _lastInjectedPid = proc.Id;
                                    StatusChanged?.Invoke("Attached to D2R (pid " + proc.Id + ")");
                                }
                                else
                                {
                                    StatusChanged?.Invoke("Injection failed: " + err);
                                    Thread.Sleep(5000);
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    StatusChanged?.Invoke("Watcher error: " + ex.Message);
                }

                Thread.Sleep(2000);
            }
        }
    }
}
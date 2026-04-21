using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace ESR_utils_App
{
    public partial class MainWindow : Window
    {
        private HotkeyConfig _cfg;
        private D2RWatcher _watcher;
        private bool _syncing = false;

        public MainWindow()
        {
            InitializeComponent();
            _cfg = ConfigManager.Load();

            TxtHotkey.Text = _cfg.Hotkey;
            TxtEmergencyStop.Text = _cfg.EmergencyStop;

            _syncing = true;
            ChkCrystals.IsChecked = _cfg.StoreCrystals;
            ChkGems.IsChecked = _cfg.StoreGems;
            ChkRunes.IsChecked = _cfg.StoreRunes;
            ChkDecals.IsChecked = _cfg.StoreDecals;
            ChkMultistocker.IsChecked = _cfg.StoreMultistocker;
            ChkNonBlankCoupons.IsChecked = _cfg.StoreNonBlankCoupons;
            ChkRerollMagic.IsChecked = _cfg.StoreRerollMagic;
            ChkRerollRare.IsChecked = _cfg.StoreRerollRare;
            UpdateRerollParentVisual();
            _syncing = false;

            _watcher = new D2RWatcher();
            _watcher.StatusChanged += OnStatusChanged;
            _watcher.Start();

            Closing += (s, e) => _watcher.Stop();
        }

        private void OnStatusChanged(string msg)
        {
            Dispatcher.Invoke(() => TxtStatus.Text = msg);
        }

        private void HotkeyBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            var tb = sender as TextBox;
            if (tb == null) return;

            e.Handled = true;

            var key = e.Key == Key.System ? e.SystemKey : e.Key;
            if (HotkeyCapture.IsModifierKey(key)) return;

            if (key == Key.Escape)
            {
                tb.Text = "";
                SaveHotkey(tb, "");
                Keyboard.ClearFocus();
                return;
            }

            var combo = HotkeyCapture.FormatHotkey(key, Keyboard.Modifiers);
            tb.Text = combo;
            SaveHotkey(tb, combo);
            Keyboard.ClearFocus();
        }

        private void SaveHotkey(TextBox tb, string combo)
        {
            if (tb == TxtHotkey) _cfg.Hotkey = combo;
            else if (tb == TxtEmergencyStop) _cfg.EmergencyStop = combo;
            ConfigManager.Save(_cfg);
        }

        private void HotkeyBox_GotFocus(object sender, RoutedEventArgs e)
        {
            var tb = sender as TextBox;
            if (tb != null) tb.Background = System.Windows.Media.Brushes.LightYellow;
        }

        private void HotkeyBox_LostFocus(object sender, RoutedEventArgs e)
        {
            var tb = sender as TextBox;
            if (tb != null) tb.Background = System.Windows.Media.Brushes.White;
        }

        private void Flag_Changed(object sender, RoutedEventArgs e)
        {
            if (_syncing) return;
            _cfg.StoreCrystals = ChkCrystals.IsChecked == true;
            _cfg.StoreGems = ChkGems.IsChecked == true;
            _cfg.StoreRunes = ChkRunes.IsChecked == true;
            _cfg.StoreDecals = ChkDecals.IsChecked == true;
            _cfg.StoreMultistocker = ChkMultistocker.IsChecked == true;
            _cfg.StoreNonBlankCoupons = ChkNonBlankCoupons.IsChecked == true;
            ConfigManager.Save(_cfg);
        }

        private void Reroll_Parent_Changed(object sender, RoutedEventArgs e)
        {
            if (_syncing) return;

            _syncing = true;
            bool? state = ChkReroll.IsChecked;
            if (state == true)
            {
                ChkRerollMagic.IsChecked = true;
                ChkRerollRare.IsChecked = true;
            }
            else if (state == false)
            {
                ChkRerollMagic.IsChecked = false;
                ChkRerollRare.IsChecked = false;
            }
            _syncing = false;

            _cfg.StoreRerollMagic = ChkRerollMagic.IsChecked == true;
            _cfg.StoreRerollRare = ChkRerollRare.IsChecked == true;
            ConfigManager.Save(_cfg);
        }

        private void Reroll_Child_Changed(object sender, RoutedEventArgs e)
        {
            if (_syncing) return;

            _syncing = true;
            UpdateRerollParentVisual();
            _syncing = false;

            _cfg.StoreRerollMagic = ChkRerollMagic.IsChecked == true;
            _cfg.StoreRerollRare = ChkRerollRare.IsChecked == true;
            ConfigManager.Save(_cfg);
        }

        private void UpdateRerollParentVisual()
        {
            bool magic = ChkRerollMagic.IsChecked == true;
            bool rare = ChkRerollRare.IsChecked == true;

            if (magic && rare) ChkReroll.IsChecked = true;
            else if (!magic && !rare) ChkReroll.IsChecked = false;
            else ChkReroll.IsChecked = null;
        }
        private void ChkReroll_Click(object sender, RoutedEventArgs e)
        {
            if (ChkReroll.IsChecked == null)
                ChkReroll.IsChecked = false;
        }
        private void About_Click(object sender, RoutedEventArgs e)
        {
            var about = new AboutWindow { Owner = this };
            about.ShowDialog();
        }
    }
}
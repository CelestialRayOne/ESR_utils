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

            ChkRerollMagicRings.IsChecked = _cfg.StoreRerollMagicRings;
            ChkRerollMagicAmulets.IsChecked = _cfg.StoreRerollMagicAmulets;
            ChkRerollMagicJewels.IsChecked = _cfg.StoreRerollMagicJewels;
            ChkRerollMagicCharms.IsChecked = _cfg.StoreRerollMagicCharms;
            ChkRerollMagicQuivers.IsChecked = _cfg.StoreRerollMagicQuivers;

            ChkRerollRareRings.IsChecked = _cfg.StoreRerollRareRings;
            ChkRerollRareAmulets.IsChecked = _cfg.StoreRerollRareAmulets;
            ChkRerollRareJewels.IsChecked = _cfg.StoreRerollRareJewels;
            ChkRerollRareCharms.IsChecked = _cfg.StoreRerollRareCharms;
            ChkRerollRareQuivers.IsChecked = _cfg.StoreRerollRareQuivers;

            ChkRerollSetUniqueRings.IsChecked = _cfg.StoreRerollSetUniqueRings;
            ChkRerollSetUniqueAmulets.IsChecked = _cfg.StoreRerollSetUniqueAmulets;
            ChkRerollSetUniqueQuivers.IsChecked = _cfg.StoreRerollSetUniqueQuivers;

            ChkSkipInventory.IsChecked = _cfg.RerollSkipInventory;
            ChkSkipStash.IsChecked = _cfg.RerollSkipStash;
            Rb169.IsChecked = _cfg.AspectRatio != "21:9";
            Rb219.IsChecked = _cfg.AspectRatio == "21:9";
            UpdateRerollParentVisual();
            _syncing = false;

            _watcher = new D2RWatcher();
            _watcher.StatusChanged += OnStatusChanged;
            _watcher.Start();

            Closing += (s, e) =>
{
    _watcher.Stop();
    try
    {
        var procs = System.Diagnostics.Process.GetProcessesByName("D2R");
        if (procs.Length > 0)
        {
            Injector.UnloadDll(procs[0].Id, out _);
        }
    }
    catch { }
};
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

            if (key == Key.Delete || key == Key.Back)
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
            _cfg.RerollSkipInventory = ChkSkipInventory.IsChecked == true;
            _cfg.RerollSkipStash = ChkSkipStash.IsChecked == true;
            ConfigManager.Save(_cfg);
        }

        private void Reroll_Parent_Changed(object sender, RoutedEventArgs e)
        {
            if (_syncing) return;

            _syncing = true;
            bool? state = ChkReroll.IsChecked;
            if (state == true)
            {
                ChkRerollMagicRings.IsChecked = true;
                ChkRerollMagicAmulets.IsChecked = true;
                ChkRerollMagicJewels.IsChecked = true;
                ChkRerollMagicCharms.IsChecked = true;
                ChkRerollMagicQuivers.IsChecked = true;
                ChkRerollRareRings.IsChecked = true;
                ChkRerollRareAmulets.IsChecked = true;
                ChkRerollRareJewels.IsChecked = true;
                ChkRerollRareCharms.IsChecked = true;
                ChkRerollRareQuivers.IsChecked = true;
                ChkRerollSetUniqueRings.IsChecked = true;
                ChkRerollSetUniqueAmulets.IsChecked = true;
                ChkRerollSetUniqueQuivers.IsChecked = true;
            }
            else if (state == false)
            {
                ChkRerollMagicRings.IsChecked = false;
                ChkRerollMagicAmulets.IsChecked = false;
                ChkRerollMagicJewels.IsChecked = false;
                ChkRerollMagicCharms.IsChecked = false;
                ChkRerollMagicQuivers.IsChecked = false;
                ChkRerollRareRings.IsChecked = false;
                ChkRerollRareAmulets.IsChecked = false;
                ChkRerollRareJewels.IsChecked = false;
                ChkRerollRareCharms.IsChecked = false;
                ChkRerollRareQuivers.IsChecked = false;
                ChkRerollSetUniqueRings.IsChecked = false;
                ChkRerollSetUniqueAmulets.IsChecked = false;
                ChkRerollSetUniqueQuivers.IsChecked = false;
            }
            _syncing = false;

            SaveAllRerollChildren();
        }

        private void Reroll_Child_Changed(object sender, RoutedEventArgs e)
        {
            if (_syncing) return;

            _syncing = true;
            UpdateRerollParentVisual();
            _syncing = false;

            SaveAllRerollChildren();
        }

        private void SaveAllRerollChildren()
        {
            _cfg.StoreRerollMagicRings = ChkRerollMagicRings.IsChecked == true;
            _cfg.StoreRerollMagicAmulets = ChkRerollMagicAmulets.IsChecked == true;
            _cfg.StoreRerollMagicJewels = ChkRerollMagicJewels.IsChecked == true;
            _cfg.StoreRerollMagicCharms = ChkRerollMagicCharms.IsChecked == true;
            _cfg.StoreRerollMagicQuivers = ChkRerollMagicQuivers.IsChecked == true;

            _cfg.StoreRerollRareRings = ChkRerollRareRings.IsChecked == true;
            _cfg.StoreRerollRareAmulets = ChkRerollRareAmulets.IsChecked == true;
            _cfg.StoreRerollRareJewels = ChkRerollRareJewels.IsChecked == true;
            _cfg.StoreRerollRareCharms = ChkRerollRareCharms.IsChecked == true;
            _cfg.StoreRerollRareQuivers = ChkRerollRareQuivers.IsChecked == true;

            _cfg.StoreRerollSetUniqueRings = ChkRerollSetUniqueRings.IsChecked == true;
            _cfg.StoreRerollSetUniqueAmulets = ChkRerollSetUniqueAmulets.IsChecked == true;
            _cfg.StoreRerollSetUniqueQuivers = ChkRerollSetUniqueQuivers.IsChecked == true;

            ConfigManager.Save(_cfg);
        }

        private void UpdateRerollParentVisual()
        {
            bool[] states = new[]
            {
                ChkRerollMagicRings.IsChecked == true,
                ChkRerollMagicAmulets.IsChecked == true,
                ChkRerollMagicJewels.IsChecked == true,
                ChkRerollMagicCharms.IsChecked == true,
                ChkRerollMagicQuivers.IsChecked == true,
                ChkRerollRareRings.IsChecked == true,
                ChkRerollRareAmulets.IsChecked == true,
                ChkRerollRareJewels.IsChecked == true,
                ChkRerollRareCharms.IsChecked == true,
                ChkRerollRareQuivers.IsChecked == true,
                ChkRerollSetUniqueRings.IsChecked == true,
                ChkRerollSetUniqueAmulets.IsChecked == true,
                ChkRerollSetUniqueQuivers.IsChecked == true,
            };

            int checkedCount = 0;
            foreach (bool s in states) if (s) checkedCount++;

            if (checkedCount == states.Length) ChkReroll.IsChecked = true;
            else if (checkedCount == 0) ChkReroll.IsChecked = false;
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

        private void Aspect_Changed(object sender, RoutedEventArgs e)
        {
            if (Rb219 != null && Rb219.IsChecked == true) _cfg.AspectRatio = "21:9";
            else _cfg.AspectRatio = "16:9";
            ConfigManager.Save(_cfg);
        }
    }
}
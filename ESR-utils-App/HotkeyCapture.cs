using System.Windows.Input;

namespace ESR_utils_App
{
    public static class HotkeyCapture
    {
        public static string FormatHotkey(Key key, ModifierKeys modifiers)
        {
            if (key == Key.None) return "";
            var parts = new System.Collections.Generic.List<string>();
            if ((modifiers & ModifierKeys.Control) != 0) parts.Add("Ctrl");
            if ((modifiers & ModifierKeys.Alt) != 0) parts.Add("Alt");
            if ((modifiers & ModifierKeys.Shift) != 0) parts.Add("Shift");
            parts.Add(key.ToString());
            return string.Join("+", parts);
        }

        public static bool IsModifierKey(Key k)
        {
            return k == Key.LeftCtrl || k == Key.RightCtrl
                || k == Key.LeftShift || k == Key.RightShift
                || k == Key.LeftAlt || k == Key.RightAlt
                || k == Key.LWin || k == Key.RWin
                || k == Key.System;
        }
    }
}
using System;
using System.IO;
using Newtonsoft.Json;

namespace ESR_utils_App
{
    public class HotkeyConfig
    {
        public string Hotkey { get; set; } = "";
        public string EmergencyStop { get; set; } = "";

        public bool StoreCrystals { get; set; } = true;
        public bool StoreGems { get; set; } = true;
        public bool StoreRunes { get; set; } = true;
        public bool StoreDecals { get; set; } = true;
        public bool StoreRerollMagic { get; set; } = false;
        public bool StoreRerollRare { get; set; } = false;
        public bool StoreMultistocker { get; set; } = true;
        public bool StoreNonBlankCoupons { get; set; } = false;
    }

    public static class ConfigManager
    {
        private static readonly string ConfigPath =
            Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "ESR-utils.json");

        public static HotkeyConfig Load()
        {
            try
            {
                if (!File.Exists(ConfigPath))
                    return new HotkeyConfig();
                var json = File.ReadAllText(ConfigPath);
                return JsonConvert.DeserializeObject<HotkeyConfig>(json) ?? new HotkeyConfig();
            }
            catch
            {
                return new HotkeyConfig();
            }
        }

        public static void Save(HotkeyConfig cfg)
        {
            try
            {
                var json = JsonConvert.SerializeObject(cfg, Newtonsoft.Json.Formatting.Indented);
                File.WriteAllText(ConfigPath, json);
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show("Save failed: " + ex.Message + "\n\nPath: " + ConfigPath);
            }
        }

        public static string ConfigFilePath => ConfigPath;
    }
}
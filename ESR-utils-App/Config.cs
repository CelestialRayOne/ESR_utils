using System;
using System.IO;
using Newtonsoft.Json;

namespace ESR_utils_App
{
    public class HotkeyConfig
    {
        public string Hotkey { get; set; } = "";
        public string EmergencyStop { get; set; } = "Escape";
        public string AspectRatio { get; set; } = "16:9";
        public bool StoreCrystals { get; set; } = true;
        public bool StoreGems { get; set; } = true;
        public bool StoreRunes { get; set; } = true;
        public bool StoreDecals { get; set; } = true;
        public bool StoreRerollMagicRings { get; set; } = false;
        public bool StoreRerollMagicAmulets { get; set; } = false;
        public bool StoreRerollMagicJewels { get; set; } = false;
        public bool StoreRerollMagicCharms { get; set; } = false;
        public bool StoreRerollMagicQuivers { get; set; } = false;

        public bool StoreRerollRareRings { get; set; } = false;
        public bool StoreRerollRareAmulets { get; set; } = false;
        public bool StoreRerollRareJewels { get; set; } = false;
        public bool StoreRerollRareCharms { get; set; } = false;
        public bool StoreRerollRareQuivers { get; set; } = false;

        public bool StoreRerollSetUniqueRings { get; set; } = false;
        public bool StoreRerollSetUniqueAmulets { get; set; } = false;
        public bool StoreRerollSetUniqueQuivers { get; set; } = false;
        public bool RerollSkipInventory { get; set; } = true;
        public bool RerollSkipStash { get; set; } = false;
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
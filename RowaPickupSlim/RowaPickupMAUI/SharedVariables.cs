using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace RowaPickupMAUI
{
    class SharedVariables
    {
        public static int SelectedPrioItem { get; set; } = 2;
        public static string SelectedPrioItemText { get; set; } = "Normal";
        public static int SourceNumber { get; set; } = 100;
        public static bool IsPickupsOnlyChecked { get; set; } = true;
        public static string ClientIpAddress { get; set; } = "127.0.0.1";
        public static int ClientPort { get; set; } = 6050;
        public static string RobotStockLocation { get; set; } = "None";
        public static bool ScanOutput { get; set; } = false;
        public static string ReadSpeed { get; set; } = "100";
        public static string OutputNumber { get; set; } = "001";
        public static TcpClient SharedTcpClient { get; set; } = new TcpClient();
        public static NetworkClient networkClient { get; set; } = new NetworkClient();

        // Centralized application data folder path
        public static string AppDataFolder { get; private set; } = "";

        // Language preference: "NL" for Dutch, "EN" for English
        public static string Language { get; set; } = "NL";
    }
    class SettingsLoader
    {
        public static async Task LoadSettingsAsync()
        {
            // Initialize centralized AppDataFolder based on platform
            string appGlobalDataDirectory = FileSystem.AppDataDirectory;
            if (DeviceInfo.Current.Platform == DevicePlatform.WinUI)
            {
                appGlobalDataDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "RowaPickup");
                if (!Directory.Exists(appGlobalDataDirectory))
                {
                    Directory.CreateDirectory(appGlobalDataDirectory);
                }
            }
            else if (DeviceInfo.Current.Platform == DevicePlatform.iOS)
            {
                // iOS-specific logic
            }
            else if (DeviceInfo.Current.Platform == DevicePlatform.Android)
            {
                // Android-specific logic
            }
            else if (DeviceInfo.Current.Platform == DevicePlatform.MacCatalyst)
            {
                // MacCatalyst-specific logic
            }
            else
            {
                throw new NotImplementedException("Platform-specific directory is not implemented for this platform.");
            }

            // Store the centralized AppDataFolder path
            SharedVariables.AppDataFolder = appGlobalDataDirectory;

            // Use AppDataFolder for config file path
            string ConfigFilePath = Path.Combine(appGlobalDataDirectory, "RowaPickup.config");

            if (File.Exists(ConfigFilePath))
            {
                try
                {
                    string[] lines = await File.ReadAllLinesAsync(ConfigFilePath);
                    foreach (var line in lines)
                    {
                        string[] parts = line.Split('=');
                        if (parts.Length == 2)
                        {
                            string key = parts[0].Trim();
                            string value = parts[1].Trim();

                            switch (key)
                            {
                                case "ClientIpAddress":
                                    SharedVariables.ClientIpAddress = value;
                                    break;
                                case "ClientPort":
                                    if (Int32.TryParse(value, out int intclientPort))
                                    {
                                        SharedVariables.ClientPort = intclientPort;
                                    }
                                    break;
                                case "RobotStockLocation":
                                    SharedVariables.RobotStockLocation = value;
                                    break;
                                case "PickupsOnly":
                                    SharedVariables.IsPickupsOnlyChecked = Convert.ToBoolean(value);
                                    break;
                                case "ScanOutput":
                                    SharedVariables.ScanOutput = Convert.ToBoolean(value);
                                    break;
                                case "ReadSpeed":
                                    SharedVariables.ReadSpeed = value;
                                    break;
                                case "OutputNumber":
                                    SharedVariables.OutputNumber = value;
                                    if (Int32.TryParse(value, out int settingInt))
                                    {
                                        // Generate a random three-digit number for the source
                                        Random random = new Random();
                                        int randomNumber = random.Next(100, 1000);

                                        // Combine settingInt and randomNumber to form SourceNumber
                                        SharedVariables.SourceNumber = settingInt * 10000 + randomNumber; break;
                                    }
                                    break;
                                case "PrioPicker":
                                    if (int.TryParse(value, out int selectedIndex))
                                    {
                                        SharedVariables.SelectedPrioItem = selectedIndex;
                                    }
                                    break;
                                case "PrioPickerText":
                                    SharedVariables.SelectedPrioItemText = value;
                                    break;
                                case "Language":
                                    SharedVariables.Language = value;
                                    break;
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Error during reading of variables: " + ex.Message);
                }

            }
            else
            {
                Debug.WriteLine("Config file not found. Using default settings.");
                // Consider setting default values or handling the absence of a config file.
            }
        }
    }
    public class MessageReceivedEventArgs : EventArgs
    {
        public string MessageType { get; }
        public string Message { get; }

        public MessageReceivedEventArgs(string messageType, string message)
        {
            MessageType = messageType;
            Message = message;
        }
    }
    public class TemporaryArticle
    {
        public string Id { get; set; } = string.Empty;
        public string Name { get; set; } = string.Empty;
        public string PackagingUnit { get; set; } = string.Empty;
        public string DosageForm { get; set; } = string.Empty;
        public int Quantity { get; set; } = 0;
    }
}

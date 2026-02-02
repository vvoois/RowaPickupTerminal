using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.IO;
using Microsoft.Maui;
using Microsoft.Maui.Devices;
using Microsoft.Maui.Storage;
using System.Diagnostics;
using Microsoft.Maui.Controls;
using CommunityToolkit.Mvvm.Messaging;

namespace RowaPickupMAUI
{
    public class SettingsViewModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged = null;
        public ICommand SaveSettingsCommand { get; private set; }

        private string _clientIpAddress = "10.x.y.10";
        public string ClientIpAddress
        {
            get => _clientIpAddress;
            set
            {
                _clientIpAddress = value;
                OnPropertyChanged(nameof(ClientIpAddress));
            }
        }
        private string _clientPort = "3045";
        public string ClientPort
        {
            get => _clientPort;
            set
            {
                _clientPort = value;
                OnPropertyChanged(nameof(ClientPort));
            }
        }

        private string _robotStockLocation = "NONE";
        public string RobotStockLocation
        {
            get => _robotStockLocation;
            set
            {
                _robotStockLocation = value;
                OnPropertyChanged(nameof(RobotStockLocation));
            }
        }

        private bool _pickupsOnly = true;
        public bool PickupsOnly
        {
            get => _pickupsOnly;
            set
            {
                _pickupsOnly = value;
                OnPropertyChanged(nameof(PickupsOnly));
            }
        }

        private bool _scanOutput = false;
        public bool ScanOutput
        {
            get => _scanOutput;
            set
            {
                _scanOutput = value;
                OnPropertyChanged(nameof(ScanOutput));
            }
        }

        private string _readSpeed = "100";
        public string ReadSpeed
        {
            get => _readSpeed;
            set
            {
                _readSpeed = value;
                OnPropertyChanged(nameof(ReadSpeed));
            }
        }

        private string _outputNumber = "002";
        public string OutputNumber
        {
            get => _outputNumber;
            set
            {
                _outputNumber = value;
                OnPropertyChanged(nameof(OutputNumber));
            }
        }

        private int _prioPickerSelectedIndex = 2;
        public int PrioPicker
        {
            get => _prioPickerSelectedIndex;
            set
            {
                _prioPickerSelectedIndex = value;
                OnPropertyChanged(nameof(PrioPicker));
            }
        }

        private string _prioPickerSelectedItem = "Normal";
        public string PrioPickerText
        {
            get => _prioPickerSelectedItem;
            set
            {
                _prioPickerSelectedItem = value;
                OnPropertyChanged(nameof(PrioPickerText));
            }
        }

        private string _passWord = "002";
        public string PassWord
        {
            get => _passWord;
            set
            {
                _passWord = value;
                OnPropertyChanged(nameof(PassWord));
            }
        }
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }


        public SettingsViewModel()
        {
            SaveSettingsCommand = new Command(async () => await SaveSettingsAsync());
            // Initialize other properties and load settings if needed
        }

        private async Task SaveSettingsAsync()
        {
            string appGlobalDataDirectory = FileSystem.AppDataDirectory;
            if (DeviceInfo.Current.Platform == DevicePlatform.WinUI)
            {
                appGlobalDataDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "RowaPickupMAUI");
                Directory.CreateDirectory(appGlobalDataDirectory);
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
            else if (DeviceInfo.Current.Platform == DevicePlatform.MacCatalyst)
            {
                // MacCatalyst-specific logic
            }
            else
            {
                throw new NotImplementedException("Platform-specific directory is not implemented for this platform.");
            }

            string ConfigFilePath = Path.Combine(appGlobalDataDirectory, "RowaPickupMaui.config");

            try
            {
                using (StreamWriter writer = new StreamWriter(ConfigFilePath))
                {
                    WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Verbinding vernieuwen..."));
                    SharedVariables.networkClient.Close();
                    await writer.WriteLineAsync($"ClientIpAddress={ClientIpAddress}");
                    SharedVariables.ClientIpAddress = ClientIpAddress;
                    await writer.WriteLineAsync($"ClientPort={ClientPort}");
                    if (Int32.TryParse(ClientPort, out int intclientPort))
                    {
                        SharedVariables.ClientPort = intclientPort;
                    }
                    await writer.WriteLineAsync($"RobotStockLocation={RobotStockLocation}");
                    SharedVariables.RobotStockLocation = RobotStockLocation;
                    await writer.WriteLineAsync($"PickupsOnly={PickupsOnly}");
                    SharedVariables.IsPickupsOnlyChecked = PickupsOnly;
                    await writer.WriteLineAsync($"ScanOutput={ScanOutput}");
                    SharedVariables.ScanOutput = ScanOutput;
                    await writer.WriteLineAsync($"ReadSpeed={ReadSpeed}");
                    SharedVariables.ReadSpeed = ReadSpeed;
                    await writer.WriteLineAsync($"OutputNumber={OutputNumber}");
                    SharedVariables.OutputNumber = OutputNumber;
                    await writer.WriteLineAsync($"PrioPicker={PrioPicker}");
                    SharedVariables.SelectedPrioItem = PrioPicker;
                    await writer.WriteLineAsync($"PrioPicker={PrioPickerText}");
                    SharedVariables.SelectedPrioItemText = PrioPickerText;
                    if (SharedVariables.ScanOutput)
                    {
                        if (SharedVariables.networkClient.tcpClient.Connected)
                        {
                            await MainThread.InvokeOnMainThreadAsync(async () =>
                            {
                               await MainPage.SendStockInfoRequest();
                            });
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Error during writing of variables " + ex.Message);
            }
        }

    }

}

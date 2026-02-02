using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Threading.Tasks;
using Microsoft.Maui.Controls.PlatformConfiguration;

namespace RowaPickupMAUI
{
	//[XamlCompilation(XamlCompilationOptions.Compile)]
	public partial class SettingsPage : ContentPage
	{
        //private IDispatcherTimer timer;
        private SettingsViewModel _viewModel;

        public SettingsPage()
		{
            InitializeComponent();
            PrioPicker.SelectedIndex = 2;
            // In your Page's code-behind or somewhere appropriate
            _viewModel = new SettingsViewModel();
            BindingContext = _viewModel;
            LoadSettingsAsync();
        }
        private void OnPickupCheckedChanged(object sender, CheckedChangedEventArgs e)
        {
            if (!e.Value)
            {
                ScanOutput.IsChecked = false;
            }
        }
        private void OnScanOutputCheckedChanged(object sender, CheckedChangedEventArgs e)
        {
            if (e.Value)
            {
                PickupsOnly.IsChecked = true;
            }
        }

        protected override void OnDisappearing()
        {
            base.OnDisappearing();
            // Assuming _viewModel is your SettingsViewModel instance
            _viewModel.SaveSettingsCommand.Execute(null);
        }


        protected override async void OnAppearing()
        {
            base.OnAppearing();
            await LoadSettingsAsync();
        }

        private void OnPasswordChanged(object sender, TextChangedEventArgs e)
        {
            var correctPassword = "ibib"; // This should be securely managed and not hardcoded
            var enteredPassword = ((Entry)sender).Text;

            if (enteredPassword == correctPassword)
            {
                // Enable the settings
                EnableSettings();
            }
            else
            {
                // Optionally, disable them again if the password is altered and incorrect
                DisableSettings();
            }
        }

        private void EnableSettings()
        {
            // Assuming you have controls like Switches, Entries, etc., for settings
            ClientIpAddress.IsEnabled = true;
            ClientPort.IsEnabled = true;
            RobotStockLocation.IsEnabled = true;
            PickupsOnly.IsEnabled = false; //Should not be enabled!
            ScanOutput.IsEnabled = true;
        }

        private void DisableSettings()
        {
            ClientIpAddress.IsEnabled = false;
            ClientPort.IsEnabled = false;
            RobotStockLocation.IsEnabled = false;
            PickupsOnly.IsEnabled = false; //Should not be enabled!
            ScanOutput.IsEnabled = false;
        }
        public async Task LoadSettingsAsync()
        {
            string appGlobalDataDirectory = FileSystem.AppDataDirectory;
            if (DeviceInfo.Current.Platform == DevicePlatform.WinUI)
            {
                appGlobalDataDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "RowaPickupMAUI");
                if (!File.Exists(appGlobalDataDirectory))
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
            else if (DeviceInfo.Current.Platform == DevicePlatform.MacCatalyst)
            {
                // MacCatalyst-specific logic
            }
            else
            {
                throw new NotImplementedException("Platform-specific directory is not implemented for this platform.");
            }

            string ConfigFilePath = Path.Combine(appGlobalDataDirectory, "RowaPickupMaui.config");

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
                                    ClientIpAddress.Text = value;
                                    SharedVariables.ClientIpAddress = value;
                                    break;
                                case "ClientPort":
                                    ClientPort.Text = value;
                                    if (Int32.TryParse(value, out int intclientPort))
                                    {
                                        SharedVariables.ClientPort = intclientPort;
                                    }
                                    break;
                                case "RobotStockLocation":
                                    RobotStockLocation.Text = value;
                                    SharedVariables.RobotStockLocation = value;
                                    break;
                                case "PickupsOnly":
                                    PickupsOnly.IsChecked = Convert.ToBoolean(value);
                                    SharedVariables.IsPickupsOnlyChecked = Convert.ToBoolean(value);
                                    break;
                                case "ScanOutput":
                                    ScanOutput.IsChecked = Convert.ToBoolean(value);
                                    SharedVariables.ScanOutput = Convert.ToBoolean(value);
                                    break;
                                case "ReadSpeed":
                                    ReadSpeed.Text = value;
                                    SharedVariables.ReadSpeed = value;
                                    break;
                                case "OutputNumber":
                                    OutputNumber.Text = value;
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
                                        PrioPicker.SelectedIndex = selectedIndex;
                                        SharedVariables.SelectedPrioItem = selectedIndex;
                                    }
                                    break;
                                case "PrioPickerText":
                                    SharedVariables.SelectedPrioItemText = value;
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
}
using CommunityToolkit.Mvvm.Messaging;
using System.Data;
using System.Net.Sockets;

namespace RowaPickupMAUI
{
    public partial class AppShell : Shell
    {
        private SettingsViewModel _viewModel;
        public AppShell()
        {
            InitializeComponent();
            SharedVariables.SharedTcpClient = new TcpClient();
            _viewModel = new SettingsViewModel();
            BindingContext = _viewModel;
        }
        private async void OnMenuItemClicked(object sender, EventArgs e)
        {
            //Hack the flyout item to be rehighlighted and selected by fluctuating between two flyout layouts.
            await Shell.Current.GoToAsync("//SettingsPage");
            await Shell.Current.GoToAsync("//MainPage");
            if (!SharedVariables.ScanOutput)
            {
                await RefreshInventory();
            }
        }

        private async Task RefreshInventory()
        {
            if (MainPage.ConnectivityState() != "none")
            {
                if (SharedVariables.networkClient.tcpClient.Connected)
                {
                    WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Voorraad vernieuwen..."));
                    string id = DateTime.UtcNow.ToString("HHmmssfff");
                    MainPage.dataTable.Clear();
                    string message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                    "<StockInfoRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\" />" +
                    "</WWKS>";

                    // Send the StockInfoRequest
                    await SharedVariables.networkClient.SendAndReceiveAsync(message);
                    await Task.Delay(2000);
                    id = DateTime.UtcNow.ToString("HHmmssfff");
                    message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                        "<StatusRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" IncludeDetails=\"True\"/>" +
                        "</WWKS>";
                    await SharedVariables.networkClient.SendAndReceiveAsync(message);
                }
            }
            else
            {
                WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Geen netwerk!"));
            }
        }
    }
}

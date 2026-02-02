//using AndroidX.AppCompat.View.Menu;
using CommunityToolkit.Mvvm.Messaging;
using Microsoft.Maui;
using Microsoft.Maui.Controls.PlatformConfiguration;
using System;
using static RowaPickupMAUI.XmlDefinitions;
using System.Data;
using System.Diagnostics;
using Microsoft.Maui.Dispatching;
using Microsoft.Maui.Graphics;
using System.Xml.Serialization;
using System.Xml.Linq;
using System.Timers;
using System.Reflection;
using System.Collections.ObjectModel;
using System.Collections;
using System.Drawing;
//using MetalPerformanceShadersGraph;
//using CoreML;
//using static AVFoundation.AVMetadataIdentifiers;

namespace RowaPickupMAUI
{
    public class UpdateStateLabel
    {
        public string Text { get; }

        public UpdateStateLabel(string text)
        {
            Text = text;
        }
    }
    public partial class MainPage : ContentPage
    {
        private CancellationTokenSource typingDelayCancellationTokenSource = new CancellationTokenSource();
        //public MainPage mainPageInstance = new MainPage();
        public NetworkClient networkClient;
        public static DataTable dataTable = new DataTable("StockInfoTable");
        private System.Timers.Timer timer;
        private string RobotState = "onbekend";
        private List<Tuple<string, string, int, Microsoft.Maui.Graphics.Color>> StockOutputRecords = new List<Tuple<string, string, int, Microsoft.Maui.Graphics.Color>>();
        //public int SourceNumber = 100;
        //SharedVariables.isPickupsOnlyChecked = false;

        private List<Article> listViewArticles; public Dictionary<string, Func<string, Task>> messageHandlers = new Dictionary<string, Func<string, Task>>();
        ObservableCollection<Article> displayedArticles = new ObservableCollection<Article>();

        /*
    private async void DisplayedArticles_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
    {
        // Optionally, introduce a slight delay
        await Task.Delay(5000); // Adjust the delay based on testing

        //update every button for current outstanding output orders
        foreach (var item in StockOutputRecords)
        {
            string referenceId = item.Item1.ToString(); // Output Id
            SendTaskInfoRequest(referenceId);
        }
    }
        */

        public MainPage()
        {
            SharedVariables.networkClient.MessageReceived += HandleNetworkClientMessageReceived;
            InitializeComponent();
            //displayedArticles.CollectionChanged += DisplayedArticles_CollectionChanged;
            WeakReferenceMessenger.Default.Register<MainPage, UpdateStateLabel>(this, (receiver, message) =>
            {
                receiver.LBConnectionState.Text = message.Text;
            });
        }

        private async void HandleNetworkClientMessageReceived(object sender, MessageReceivedEventArgs e)
        {
            switch (e.MessageType)
            {
                case "HelloResponse":
                    await HandleHelloResponseAsync(sender, e);
                    break;
                case "StatusResponse":
                    await HandleStatusResponseAsync(sender, e);
                    break;
                case "StockInfoResponse":
                    await HandleStockInfoResponseAsync(sender, e);
                    break;
                case "OutputResponse":
                    await HandleStockOutputResponseAsync(sender, e);
                    break;
                case "OutputMessage":
                    await HandleStockOutputMessageAsync(sender, e);
                    break;
                case "InputMessage":
                    //await HandleInputMessageAsync(sender, e);
                    break;
                case "TaskInfoResponse":
                    await HandleTaskInfoResponseMessageAsync(sender, e);
                    break;
                //messageHandlers["InputMessage"] = new Func<string, Task>(async response => await HandleStockInputMessageAsync(response));
                default:
                    Debug.WriteLine("Message not supported [" + e.MessageType + "]");
                    break;
            }
        }
        public static string ConnectivityState()
        {
            NetworkAccess accessType = Connectivity.Current.NetworkAccess;
            switch (accessType)
            {
                case NetworkAccess.Internet:
                    return "Internet";
                case NetworkAccess.ConstrainedInternet:
                    return "Limited";
                case NetworkAccess.Local:
                    return "local";
                case NetworkAccess.Unknown:
                    return "unknown";
                case NetworkAccess.None:
                    return "none";
            }
            return "none";
        }


        private async Task SendHelloAsync()
        {
            // Send any necessary initialization messages after recovering the connection
            if (SharedVariables.networkClient.tcpClient.Connected)
            {
                string id = DateTime.UtcNow.ToString("HHmmssfff");
                Version assemblyVersion = Assembly.GetExecutingAssembly().GetName()?.Version ?? new Version(0, 0, 0); 
                string versionString = assemblyVersion?.ToString() ?? string.Empty;
                string hellostring = "<WWKS Version=\"2.0\" TimeStamp=\"2023-11-30T14:53:51Z\">" +
                    "<HelloRequest Id=\"" + id + "\">" +
                    "<Subscriber Id=\"" + SharedVariables.SourceNumber.ToString() + "\" Type=\"IMS\" Manufacturer=\"Becton Dickenson Netherlands\" ProductInfo=\"RowaStockView\" VersionInfo=\"" + versionString + "\">" +
                    "<Capability Name=\"KeepAlive\" /><Capability Name=\"Status\" />" +
                    "<Capability Name=\"Input\" />" +
                    "<Capability Name=\"InitiateInput\" />" +
                    "<Capability Name=\"ArticleMaster\" />" +
                    "<Capability Name=\"StockDelivery\" />" +
                    "<Capability Name=\"StockInfo\" />" +
                    "<Capability Name=\"Output\" />" +
                    "<Capability Name=\"OutputInfo\" />" +
                    "<Capability Name=\"TaskInfo\" />" +
                    "<Capability Name=\"TaskCancel\" />" +
                    "<Capability Name=\"Configuration\" />" +
                    "<Capability Name=\"StockLocationInfo\" />" +
                    "<Capability Name=\"ArticleInfo\" />" +
                    "<Capability Name=\"Infeed\" />" +
                    "<Capability Name=\"Unprocessed\" />" +
                    "</Subscriber></HelloRequest></WWKS>";


                // Send the HelloRequest and wait for the HelloResponse
                await SharedVariables.networkClient.SendAndReceiveAsync(hellostring);
                //Debug.WriteLine("Returned HelloResponse: " + helloResponse);
            }
        }
        // Handle HelloResponse messages
        private async Task HandleHelloResponseAsync(object sender, MessageReceivedEventArgs e)
        {
            await MainThread.InvokeOnMainThreadAsync(async () =>
            {
                Debug.WriteLine("Returned HelloResponse: " + e.Message);
                if (e.Message.Length > 0)
                {
                    await Task.Delay(2000);
                    await SendStatusRequest();
                    await Task.Delay(2000);
                    await SendStockInfoRequest();
                    //Debug.WriteLine("Returned StockInfoResponse: " + stockInfoResponse);
                }
            });

        }

        public async static Task SendStockInfoRequest()
        {
            if (!SharedVariables.ScanOutput)
            {
                WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Voorraad verversen..."));
                string id = DateTime.UtcNow.ToString("HHmmssfff");

                string message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                "<StockInfoRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"False\">" +
                    "<Criteria StockLocationId=\"" + SharedVariables.RobotStockLocation + "\" />" +
                "</StockInfoRequest>" +
                "</WWKS>";

                // Send the StockInfoRequest
                await SharedVariables.networkClient.SendAndReceiveAsync(message);
            }
        }
        // Utility method to extract the message type from the response

        private async Task SendStatusRequest()
        {
            await Task.Delay(2000);
            string id = DateTime.UtcNow.ToString("HHmmssfff");
            string message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                "<StatusRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" IncludeDetails=\"True\"/>" +
                "</WWKS>";
            await SharedVariables.networkClient.SendAndReceiveAsync(message);

        }
        private async Task HandleStatusResponseAsync(object sender, MessageReceivedEventArgs e)
        {
            try
            {
                await MainThread.InvokeOnMainThreadAsync(async () =>
                {
                    XDocument doc = XDocument.Parse(e.Message);
                    XElement? statusResponseXml = doc.Descendants("StatusResponse").FirstOrDefault();

                    if (statusResponseXml != null)
                    {
                        var storageSystem = statusResponseXml.Descendants("Component")
                            .Where(comp => comp.Attribute("Type")?.Value == "StorageSystem")
                            .FirstOrDefault();

                        if (storageSystem != null)
                        {
                            string description = storageSystem.Attribute("Description")?.Value ?? string.Empty;
                            string state = storageSystem.Attribute("State")?.Value ?? string.Empty;

                            if (!string.IsNullOrEmpty(description) && !string.IsNullOrEmpty(state))
                            {
                                int robotNumber;
                                if (TryExtractRobotNumber(description, out robotNumber))
                                {
                                    string componentState = (state == "Ready") ? "Gereed" : "Inactief";
                                    string robotState = $"(ROB{robotNumber} [{componentState}])";
                                    Debug.WriteLine("Filtered Result: " + robotState);
                                    RobotState = robotState;
                                }
                            }
                        }
                    }
                });
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Error processing status response: " + ex.Message);
            }
        }

        static bool TryExtractRobotNumber(string description, out int robotNumber)
        {
            if (int.TryParse(description.Split(' ').Last(), out robotNumber))
            {
                return true;
            }

            return false;
        }

        private async Task HandleStockInfoResponseAsync(object sender, MessageReceivedEventArgs e)
        {
            await MainThread.InvokeOnMainThreadAsync(async () =>
            {

                var serializer = new XmlSerializer(typeof(StockInfoResponse));
                string stockInfoResponse = e.Message;
                string extractedXml = "";
                // Find the opening <StockInfoResponse> tag
                int start = stockInfoResponse.IndexOf("<StockInfoResponse");

                XDocument xDoc = XDocument.Parse(stockInfoResponse);

                // Finding the StockInfoResponse element
                var stockInfoResponseElement = xDoc.Descendants().FirstOrDefault(e => e.Name.LocalName == "StockInfoResponse");

                if (stockInfoResponseElement != null)
                {
                    extractedXml = stockInfoResponseElement.ToString();
                }
                else
                {
                    return;
                    // Handle the case where StockInfoResponse is not found
                }

                stockInfoResponse = extractedXml;
                using (var reader = new StringReader(stockInfoResponse))
                {
                    try
                    {
                        var cstockInfoResponse = (StockInfoResponse)serializer.Deserialize(reader);

                            // Create a DataTable to store the deserialized data
                        if (SharedVariables.ScanOutput)
                        {
                            dataTable.Clear();
                        }
                        AddColumnIfNotExists(dataTable, "Id", typeof(string));
                        AddColumnIfNotExists(dataTable, "Name", typeof(string));
                        AddColumnIfNotExists(dataTable, "DosageForm", typeof(string));
                        AddColumnIfNotExists(dataTable, "PackagingUnit", typeof(string));
                        AddColumnIfNotExists(dataTable, "Quantity", typeof(int));
                        //AddColumnIfNotExists(dataTable, "MaxSubItemQuantity", typeof(int));

                        //This routine is the fallback when a StockInfoResponse returns empty: We always request to get an answer, so if it is empty,
                        //for which article did we requested it?
                        //Check if the StockInfoRecord matches an outputID -> the Id's are always timestamp based, so if our 
                        //output order records has an identical id, this was the article code we were requesting the quantity for
                        //otherwise, get the articleID of the from order ID as reference, this is for responses that do not contain an output request id.
                        string referenceArticle = string.Empty; // Initialize to an empty string
                        if (cstockInfoResponse != null)
                        {
                            if (cstockInfoResponse.Id.Contains("["))
                            {
                                // Find the index of the first "[" character
                                int startIndex = cstockInfoResponse.Id.IndexOf("[");

                                // Find the index of the first "]" character, starting from the startIndex
                                int endIndex = cstockInfoResponse.Id.IndexOf("]", startIndex);

                                // Ensure both "[" and "]" are found and in the correct order
                                if (startIndex != -1 && endIndex != -1 && endIndex > startIndex)
                                {
                                    // Extract the content within the "[ ]" characters
                                    referenceArticle = cstockInfoResponse.Id.Substring(startIndex + 1, endIndex - startIndex - 1);

                                    // Also update cstockInfoResponse.Id to remove the extracted part if needed
                                    // If you wish to keep the part before the "[", you can do the following:
                                    string modifiedId = cstockInfoResponse.Id.Substring(0, startIndex);
                                    // Update the Id with the modified value
                                    cstockInfoResponse.Id = modifiedId;
                                    var outputRecord = StockOutputRecords.FirstOrDefault(item => item.Item1 == cstockInfoResponse.Id.ToString());
                                    if (outputRecord != null)
                                    {
                                        //Output order exists for article
                                        string CurrentArticleId = outputRecord.Item2;
                                        int CurrentArticleQuantity = outputRecord.Item3;
                                        var existingRows = dataTable.Select($"Id = '{CurrentArticleId}'");

                                        if (existingRows.Length > 0)
                                        {
                                            // Article exists, update the Quantity only
                                            var rowToUpdate = existingRows[0];
                                            rowToUpdate["Quantity"] = CurrentArticleQuantity; // Update the quantity with the new value

                                            var articleItem = displayedArticles.FirstOrDefault(item => item.Id == CurrentArticleId);
                                            if (articleItem != null)
                                            {
                                                var articleIndex = displayedArticles.IndexOf(articleItem);
                                                if (articleIndex != -1)
                                                {
                                                    var existingArticle = displayedArticles[articleIndex];
                                                    Microsoft.Maui.Graphics.Color targetColor = existingArticle.ButtonColor;
                                                    var matchingRecord = StockOutputRecords.FirstOrDefault(record => record.Item2 == existingArticle.Id.ToString());
                                                    if (matchingRecord != null)
                                                    {
                                                        targetColor = matchingRecord.Item4;
                                                    }
                                                    if (cstockInfoResponse.Articles != null)
                                                    {
                                                        if (cstockInfoResponse.Articles.Count == 0)
                                                        {
                                                            //If the StockInforesponse returned 0 articles, it was most likely because StockInfoRequest was 
                                                            //done for one article that returned no count, so we have to set the quantity to 0
                                                            CurrentArticleQuantity = 0;
                                                        }
                                                    }
                                                    // Create a new Article instance with the existing data but new color
                                                    var updatedArticle = new Article
                                                    {
                                                        Id = existingArticle.Id,
                                                        Name = existingArticle.Name,
                                                        DosageForm = existingArticle.DosageForm,
                                                        PackagingUnit = existingArticle.PackagingUnit,
                                                        Quantity = CurrentArticleQuantity,
                                                        ButtonColor = int.Parse(CurrentArticleQuantity.ToString()) == 0 ? Colors.Grey : targetColor
                                                    };

                                                    // Update the collection
                                                    displayedArticles[articleIndex] = updatedArticle;

                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        //Order does not exist, refresh record for reference article in the table if present
                                        ChangeRowProperties(referenceArticle, Colors.Grey, true);
                                    }
                                }
                            }
                            if (cstockInfoResponse.Articles != null)
                            {
                                if (cstockInfoResponse.Articles.Count > 0)
                                {
                                    foreach (var article in cstockInfoResponse.Articles)

                                    {
                                        var existingRows = dataTable.Select($"Id = '{article.Id}'");
                                        WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Verwerken voorraadtransactie..."));


                                        if (existingRows.Length > 0)
                                        {
                                            // Article exists, update the Quantity only
                                            var rowToUpdate = existingRows[0];
                                            rowToUpdate["Quantity"] = article.Quantity; // Update the quantity with the new value
                                            var articleItem = displayedArticles.FirstOrDefault(item => item.Id == article.Id);
                                            if (articleItem != null)
                                            {
                                                var articleIndex = displayedArticles.IndexOf(articleItem);
                                                if (articleIndex != -1)
                                                {
                                                    var existingArticle = displayedArticles[articleIndex];

                                                    // Create a new Article instance with the existing data but new color
                                                    var updatedArticle = new Article
                                                    {
                                                        Id = existingArticle.Id,
                                                        Name = existingArticle.Name,
                                                        DosageForm = existingArticle.DosageForm,
                                                        PackagingUnit = existingArticle.PackagingUnit,
                                                        Quantity = article.Quantity,
                                                        ButtonColor = int.Parse(article.Quantity.ToString()) == 0 ? Colors.Grey : existingArticle.ButtonColor
                                                    };

                                                    // Update the collection
                                                    displayedArticles[articleIndex] = updatedArticle;

                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Article does not exist, add a new row
                                            var newRow = dataTable.NewRow();
                                            newRow["Id"] = article.Id;
                                            newRow["Name"] = article.Name;
                                            newRow["DosageForm"] = article.DosageForm;
                                            newRow["PackagingUnit"] = article.PackagingUnit;
                                            newRow["Quantity"] = article.Quantity;
                                            // In case the MaxSubItemQuantity column needs to be re-added:
                                            // newRow["MaxSubItemQuantity"] = article.MaxSubItemQuantity;

                                            dataTable.Rows.Add(newRow);
                                        }
                                    }
                                    try
                                    {
                                        /*if ((bool)SharedVariables.IsPickupsOnlyChecked)
                                        {*/
                                            string searchTerm = "RoWa"; // Replace with your search term
                                            DataRow[] searchResults = dataTable.Select($"Id LIKE '%{searchTerm}%'");

                                            var searchResultsList = searchResults.Cast<DataRow>().Select(row =>
                                                new Article
                                                {
                                                    Id = row["Id"]?.ToString() ?? string.Empty,
                                                    Name = row["Name"]?.ToString() ?? string.Empty,
                                                    DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                                                    PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                                                    Quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty),
                                                }).ToList();

                                            // Bind the search results to myListView
                                            //myConnectionView.ItemsSource = searchResultsList;
                                            // Use the Dispatcher to update UI elements from the UI thread
                                            // await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                                            //{
                                            await MainThread.InvokeOnMainThreadAsync(() =>
                                            {
                                                listViewArticles = searchResultsList;
                                                string articleId = string.Empty;
                                                int articleQty = 0;

                                                var initialData = listViewArticles.Take(30); // Load first 80 items
                                                if (SharedVariables.ScanOutput)
                                                {
                                                    //Clear table when only one item should be listed
                                                    displayedArticles.Clear();
                                                }
                                                foreach (var newItem in initialData)
                                                {
                                                    var existingArticle = displayedArticles.FirstOrDefault(a => a.Id == newItem.Id);
                                                    WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Verwerken voorraadtransactie..."));

                                                    if (existingArticle != null)
                                                    {
                                                        // Assuming Article implements INotifyPropertyChanged, update properties directly
                                                        existingArticle.Name = newItem.Name;
                                                        existingArticle.DosageForm = newItem.DosageForm;
                                                        existingArticle.PackagingUnit = newItem.PackagingUnit;
                                                        existingArticle.Quantity = newItem.Quantity;
                                                        existingArticle.ButtonColor = int.Parse(newItem.Quantity.ToString()) == 0 ? Colors.Grey : existingArticle.ButtonColor;
                                                        // Any other properties that need updating
                                                    }
                                                    else
                                                    {
                                                        displayedArticles.Add(newItem);
                                                        if (articleId == null)
                                                        {
                                                            articleId = newItem.Id;
                                                            articleQty = newItem.Quantity;
                                                        }
                                                    }

                                                }
                                                if (myConnectionView != null)
                                                {
                                                    WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Verwerken voorraadtransactie..."));
                                                    myConnectionView.ItemsSource = displayedArticles;

                                                }
                                                else
                                                {
                                                    // Handle the null case, maybe log an error or set a breakpoint to understand why it's null
                                                    Debug.WriteLine("myConnectionView = null!");
                                                }

                                                // myConnectionView.ItemsSource = listViewArticles;

                                                bool hasRecords = searchResultsList != null && searchResultsList.Count > 0;

                                                // Set SearchButton.IsEnabled based on the result
                                                if (!SharedVariables.ScanOutput)
                                                {
                                                    SearchButton.IsEnabled = hasRecords;
                                                    SearchPhrase.IsEnabled = hasRecords;
                                                }
                                                else
                                                {
                                                    if (!string.IsNullOrEmpty(articleId))
                                                    {
                                                        var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item2.ToString() == articleId);
                                                        string referenceArticle = "";
                                                        string referenceId = "";
                                                        if (existingItem != null)
                                                        {
                                                            //Check if current output queue does not contain request for pickup
                                                            referenceArticle = existingItem.Item2.ToString();
                                                            referenceId = existingItem.Item1.ToString();
                                                        }

                                                        if (referenceArticle != articleId)
                                                        {
                                                            //if article is not in queue, then request output
                                                            SendOutputRequest(articleId, articleQty.ToString());
                                                        }
                                                        else
                                                        {
                                                            //SendTaskInfoRequest(referenceId);
                                                        }
                                                    } else
                                                    {
                                                        if (SharedVariables.ScanOutput && displayedArticles.Count == 1)
                                                        {
                                                            SendOutputOrder(displayedArticles[0]); // Placeholder method, replace with actual implementation
                                                        }
                                                    }
                                                }
                                            });
                                        /*}
                                        else
                                        {
                                            //myConnectionView.ItemsSource = cstockInfoResponse.Articles;
                                            // Use the Dispatcher to update UI elements from the UI thread
                                            //await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                                            //{
                                            await MainThread.InvokeOnMainThreadAsync(() =>
                                            {
                                                //listViewArticles = cstockInfoResponse.Articles;
                                                string articleId = string.Empty;
                                                int articleQty = 0;


                                                listViewArticles = cstockInfoResponse.Articles
                                                        .Select(article => new Article
                                                        {
                                                            Id = article.Id,
                                                            Name = article.Name,
                                                            DosageForm = article.DosageForm,
                                                            PackagingUnit = article.PackagingUnit,
                                                            Quantity = article.Quantity
                                                        }).ToList();
                                                var initialData = listViewArticles.Take(30); // Load first 20 items, for example
                                                if (SharedVariables.ScanOutput)
                                                {
                                                    //Clear table when only one item should be listed
                                                    displayedArticles.Clear();
                                                }
                                                foreach (var newItem in initialData)
                                                {
                                                    var existingArticle = displayedArticles.FirstOrDefault(a => a.Id == newItem.Id);
                                                    WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Verwerken voorraadtransactie..."));
                                                    if (existingArticle != null)
                                                    {
                                                        // Assuming Article implements INotifyPropertyChanged, update properties directly
                                                        existingArticle.Name = newItem.Name;
                                                        existingArticle.DosageForm = newItem.DosageForm;
                                                        existingArticle.PackagingUnit = newItem.PackagingUnit;
                                                        existingArticle.Quantity = newItem.Quantity;
                                                        // Any other properties that need updating
                                                    }
                                                    else
                                                    {
                                                        displayedArticles.Add(newItem);
                                                        //Only create an output task for a new item
                                                        if (articleId == null)
                                                        {
                                                            articleId = newItem.Id;
                                                            articleQty = newItem.Quantity;
                                                        }
                                                    }

                                                }
                                                if (myConnectionView != null)
                                                {

                                                    myConnectionView.ItemsSource = displayedArticles;
                                                }
                                                else
                                                {
                                                    // Handle the null case, maybe log an error or set a breakpoint to understand why it's null
                                                    Debug.WriteLine("myConnectionView = null!");
                                                }
                                                bool hasRecords = cstockInfoResponse.Articles != null && cstockInfoResponse.Articles.Count > 0;

                                                // Set SearchButton.IsEnabled based on the result
                                                if (!SharedVariables.ScanOutput)
                                                {
                                                    SearchButton.IsEnabled = hasRecords;
                                                    SearchPhrase.IsEnabled = hasRecords;
                                                }
                                                else
                                                {
                                                    if (!string.IsNullOrEmpty(articleId))
                                                    {
                                                        var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item2.ToString() == articleId);
                                                        string referenceArticle = "";
                                                        string referenceId = "";
                                                        if (existingItem != null)
                                                        {
                                                            //Check if current output queue does not contain request for pickup
                                                            referenceArticle = existingItem.Item2.ToString();
                                                            referenceId = existingItem.Item1.ToString();
                                                        }

                                                        if (referenceArticle != articleId)
                                                        {
                                                            //if article is not in queue, then request output
                                                            SendOutputRequest(articleId, articleQty.ToString());
                                                        }
                                                        else
                                                        {
                                                            //SendTaskInfoRequest(referenceId);
                                                        }
                                                    }
                                                }
                                            });
                                        }*/
                                    }
                                    catch (Exception ex)
                                    {
                                        Debug.WriteLine("Error processing stock info response: " + ex.Message);
                                    }
                                }
                            }
                        }
                    }
                    catch (InvalidOperationException ex)
                    {
                        Console.WriteLine(ex.Message);
                        if (ex.InnerException != null)
                        {
                            Console.WriteLine(ex.InnerException.Message);
                        }
                    }


                }
            });
        }
        private static async void SendOutputRequest(string articleId, string articleQty)
        {
            string id = DateTime.UtcNow.ToString("HHmmssfff");
            WeakReferenceMessenger.Default.Send(new UpdateStateLabel($"Uitgifte: {articleId}, aantal: {articleQty}"));
            string outputRequest = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                    "<OutputRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\"><Details OutputDestination=\"" + SharedVariables.OutputNumber + "\" Priority=\"" + SharedVariables.SelectedPrioItemText + "\" />" +
                                    "<Criteria ArticleId=\"" + articleId + "\" Quantity=\"" + articleQty + "\" />" +
                                    "</OutputRequest>" +
                                    "</WWKS>";

            //await SharedVariables.networkClient.SendAndReceiveAsync(outputRequest);
            await SharedVariables.networkClient.SendAndReceiveAsync(outputRequest);
        }
        private static async void SendTaskInfoRequest(string taskId)
        {
            string id = DateTime.UtcNow.ToString("HHmmssfff");
            string taskInfoRequest = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                "<TaskInfoRequest Id=\"" + id + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludetaskDetails=\"True\">" +
                    "<Task Type=\"Output\" Id=\"" + taskId + "\"/>" +
                "</TaskInfoRequest>" +
            "</WWKS>";
            await SharedVariables.networkClient.SendAndReceiveAsync(taskInfoRequest);
        }

        private void OnRemainingItemsThresholdReached(object sender, EventArgs e)
        {
            LoadMoreData(); // Your method to add more items to the collection
        }


        private async void LoadMoreData()
        {
            await MainThread.InvokeOnMainThreadAsync(async () =>
            {
                var nextItems = listViewArticles.Skip(displayedArticles.Count).Take(20);
                foreach (var item in nextItems)
                {
                    displayedArticles.Add(item);
                }

                if (displayedArticles.Count == listViewArticles.Count)
                {
                    myConnectionView.RemainingItemsThreshold = -1; // Stop loading more data
                }
            });
        }

        private void AddColumnIfNotExists(DataTable dataTable, string columnName, Type columnType)
        {
            if (!dataTable.Columns.Contains(columnName))
            {
                dataTable.Columns.Add(columnName, columnType);
            }
        }

        async void OnCollectionViewSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
        }

        async void OnOutputClicked(object sender, EventArgs e)
        {
            if (sender is Button button && button.BindingContext != null)
            {
                var item = button.BindingContext;
                myConnectionView.SelectedItem = item;  // Set the selected item
                var selectedItem = myConnectionView.SelectedItem;
                Debug.WriteLine("Selected Article ID:" + selectedItem.GetType().GetProperty("Id")?.GetValue(selectedItem, null));

                if (selectedItem != null)
                {
                    var propertyValue = selectedItem.GetType().GetProperty("Id")?.GetValue(selectedItem, null);

                    // Convert the property value to a string, or use string.Empty if propertyValue is null.
                    string articleId = propertyValue?.ToString() ?? string.Empty;
                    //string articleId = selectedItem.GetType().GetProperty("Id")?.GetValue(selectedItem, null).ToString() ?? string.Empty;
                    int articleQty = 0;
                    bool isInt = int.TryParse(selectedItem.GetType().GetProperty("Quantity")?.GetValue(selectedItem, null)?.ToString() ?? string.Empty, out articleQty);
                    var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item2.ToString() == articleId);
                    string id = DateTime.UtcNow.ToString("HHmmssfff");
                    if (existingItem == null)
                    {


                        if (!string.IsNullOrEmpty(articleId.ToString()))
                        {
                            var name = selectedItem.GetType().GetProperty("Name")?.GetValue(selectedItem, null);
                            var packagingUnit = selectedItem.GetType().GetProperty("PackagingUnit")?.GetValue(selectedItem, null);
                            var dosageForm = selectedItem.GetType().GetProperty("DosageForm")?.GetValue(selectedItem, null);
                            isInt = int.TryParse(selectedItem.GetType().GetProperty("Quantity")?.GetValue(selectedItem, null)?.ToString() ?? string.Empty, out articleQty);
                            string priorityValue = SharedVariables.SelectedPrioItemText;
                            if (articleQty > 0)
                            {
                                bool answer = await DisplayAlert("Uitladen", articleId + " uitladen?", "Ja", "Nee");
                                if (answer)
                                {
                                    button.BackgroundColor = Colors.Blue;
                                    // User confirmed action
                                }
                                else
                                {
                                    return;
                                }
                                SendOutputRequest(articleId, articleQty.ToString());
                            }
                        }
                    }
                    else
                    {
                        string existingOrderId = existingItem.Item1.ToString();
                        SendTaskInfoRequest(existingOrderId);
                    }
                }
            }
        }

        private async Task HandleStockOutputResponseAsync(object sender, MessageReceivedEventArgs e)
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                string stockOutputResponse = e.Message.ToString();
                //ComboBoxItem selectedPrioItem = (ComboBoxItem)SettingsPage.PrioPicker.SelectedItem;
                string selectedPrioItem = SharedVariables.SelectedPrioItemText;
                Debug.WriteLine("Output Status = " + stockOutputResponse);

                XDocument doc = XDocument.Parse(stockOutputResponse);

                // Extract OutputResponse element
                XElement? outputResponseElement = doc.Root?.Elements("OutputResponse").FirstOrDefault();

                if (outputResponseElement != null)
                {
                    string status = outputResponseElement.Element("Details")?.Attribute("Status")?.Value ?? string.Empty;
                    string articleId = outputResponseElement.Element("Criteria")?.Attribute("ArticleId")?.Value ?? string.Empty;
                    int articleQty = 0;
                    bool isInt = int.TryParse(outputResponseElement.Element("Criteria")?.Attribute("Quantity")?.Value.ToString(), out articleQty);
                    // Handle "Queued" status
                    if (status == "Queued")
                    {
                        // Change row color to blue
                        ChangeRowProperties(articleId, Colors.Blue, false);

                        // Store Id, ArticleId, and Quantity in the 2D array
                        UpdateArray(outputResponseElement.Attribute("Id")?.Value ?? string.Empty, articleId, articleQty, Colors.Blue);
                    }
                    if (status == "Rejected")
                    {
                        // Change row color to blue
                        ChangeRowProperties(articleId, Colors.Red, false);
                    }
                }
            });
        }

        private async void ChangeRowProperties(string articleId, Microsoft.Maui.Graphics.Color color, bool ChangePackQuantity)
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                var articleItem = displayedArticles.FirstOrDefault(item => item.Id == articleId);
                if (articleItem != null)
                {
                    var articleIndex = displayedArticles.IndexOf(articleItem);
                    if (articleIndex != -1)
                    {
                        var existingArticle = displayedArticles[articleIndex];
                        var PackQuantity = 0;// existingArticle.Quantity;
                        if (ChangePackQuantity == true)
                        {
                            PackQuantity = existingArticle.Quantity;
                        }
                        // Create a new Article instance with the existing data but new color
                        if (existingArticle.Quantity == 0 && color != Colors.Grey)
                        {
                            color = Colors.Grey;
                        }
                        var updatedArticle = new Article
                        {
                            Id = existingArticle.Id,
                            Name = existingArticle.Name,
                            DosageForm = existingArticle.DosageForm,
                            PackagingUnit = existingArticle.PackagingUnit,
                            Quantity = (existingArticle.Quantity - PackQuantity),
                            ButtonColor = color // Set the new color
                        };

                        // Update the collection
                        displayedArticles[articleIndex] = updatedArticle;

                    }
                }
            });
        }

        private void ForceRefreshCollectionView()
        {
            var currentItems = myConnectionView.ItemsSource;
            myConnectionView.ItemsSource = null;
            myConnectionView.ItemsSource = currentItems;
        }

        private void UpdateArray(string outputId, string articleId, int quantity, Microsoft.Maui.Graphics.Color color)
        {
            // This is the array that contains the output orders started from within this program
            // It will be used to update the state and color of the buttons.
            var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item1 == outputId);

            if (existingItem != null)
            {
                StockOutputRecords[StockOutputRecords.IndexOf(existingItem)] = Tuple.Create(outputId, articleId, quantity, color);
            }
            else
            {
                StockOutputRecords.Add(Tuple.Create(outputId, articleId, quantity, color));
            }
        }

        //Based on the output message, button colors will be updated, if the request was initiated here.
        private async Task HandleStockOutputMessageAsync(object sender, MessageReceivedEventArgs e)
        {
            await MainThread.InvokeOnMainThreadAsync(async () =>
            {
                string stockOutputMessage = e.Message;
                string selectedPrioItem = SharedVariables.SelectedPrioItemText;
                Debug.WriteLine("Output Status = " + stockOutputMessage);

                XDocument doc = XDocument.Parse(stockOutputMessage);

                // Extract OutputMessage element
                XElement? outputMessageElement = doc.Root?.Elements("OutputMessage").FirstOrDefault();

                if (outputMessageElement != null)
                {
                    string orderId = outputMessageElement.Attribute("Id")?.Value ?? string.Empty;
                    string status = outputMessageElement.Element("Details")?.Attribute("Status")?.Value ?? string.Empty;

                    // Get articleId from the Article element
                    XElement? articleElement = outputMessageElement.Element("Article");
                    string articleId = articleElement?.Attribute("Id")?.Value ?? string.Empty;
                    int quantity = 1;
                    string id;
                    string message;
                    // Use articleId if not null or empty; otherwise, use orderId
                    if (string.IsNullOrEmpty(articleId))
                    {
                        var outputRecord = StockOutputRecords.FirstOrDefault(item => item.Item1 == orderId);
                        if (outputRecord != null)
                        {
                            articleId = outputRecord.Item2;
                            quantity = outputRecord.Item3;
                            // Change font color based on status for our own output order!
                            if (status == "Incomplete")
                            {
                                ChangeRowProperties(articleId, Colors.Red, false);
                                UpdateArray(orderId, articleId, quantity, Colors.Red);
                                //Send StockInfoRequest on article
                                id = DateTime.UtcNow.ToString("HHmmssfff");

                                //Send a stockinforequest using the output ID from the specific article as StockInfoRequest Id:
                                //If the StockInfoResponse is empty, we can use the Id to see what article code was requested from the output request we still have on the record.
                                message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + outputRecord.Item1.ToString() + "[" + articleId.ToString() + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                "<Criteria ArticleId=\"" + articleId.ToString() + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\" />" +
                                "</StockInfoRequest>" +
                                "</WWKS>";

                                // Send the StockInfoRequest
                                await SharedVariables.networkClient.SendAndReceiveAsync(message);
                            }
                            else
                            {
                                UpdateArray(orderId, articleId, 0, Colors.Green);
                                ChangeRowProperties(articleId, Colors.Green, true);
                            }
                            //ForceRefresh();
                        }
                        else
                        {
                            if (!string.IsNullOrWhiteSpace(articleId))
                            {
                                id = DateTime.UtcNow.ToString("HHmmssfff");

                                //Send a stockinforequest using the output ID from the specific article as StockInfoRequest Id:
                                //If the StockInfoResponse is empty, we can use the Id to see what article code was requested from the output request we still have on the record.
                                message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + id + "[" + articleId.ToString() + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                "<Criteria ArticleId=\"" + articleId.ToString() + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\"/>" +
                                "</StockInfoRequest>" +
                                "</WWKS>";

                                //if (SharedVariables.IsPickupsOnlyChecked == true && articleId.Contains("RoWa"))
                                if (articleId.Contains("RoWa"))
                                {
                                        // Send the StockInfoRequest
                                        await SharedVariables.networkClient.SendAndReceiveAsync(message);
                                }
                                /*
                                if (SharedVariables.IsPickupsOnlyChecked == false)
                                {
                                    // Send the StockInfoRequest
                                    await SharedVariables.networkClient.SendAndReceiveAsync(message);
                                }*/
                            }
                            Debug.WriteLine($"ItemId {orderId} not found in StockOutputRecords");
                        }

                    }
                    else
                    {


                        var outputRecord = StockOutputRecords.FirstOrDefault(item => item.Item1 == orderId);
                        if (outputRecord != null)
                        {
                            articleId = outputRecord.Item2;
                            quantity = outputRecord.Item3;
                            // Change font color based on status for our own output order!
                            if (status == "Incomplete")
                            {
                                ChangeRowProperties(articleId, Colors.Red, false);
                                UpdateArray(orderId, articleId, quantity, Colors.Red);
                                //Send StockInfoRequest on article
                                id = DateTime.UtcNow.ToString("HHmmssfff");

                                //Send a stockinforequest using the output ID from the specific article as StockInfoRequest Id:
                                //If the StockInfoResponse is empty, we can use the Id to see what article code was requested from the output request we still have on the record.
                                message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + outputRecord.Item1.ToString() + "[" + articleId.ToString() + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                "<Criteria ArticleId=\"" + articleId.ToString() + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\" />" +
                                "</StockInfoRequest>" +
                                "</WWKS>";

                                // Send the StockInfoRequest
                                await SharedVariables.networkClient.SendAndReceiveAsync(message);
                            }
                            else
                            {
                                UpdateArray(orderId, articleId, 0, Colors.Green);
                                ChangeRowProperties(articleId, Colors.Green, true);
                            }
                            //ForceRefresh();
                        }
                        else
                        {
                            if (!string.IsNullOrWhiteSpace(articleId))
                            {
                                id = DateTime.UtcNow.ToString("HHmmssfff");

                                //Send a stockinforequest using the output ID from the specific article as StockInfoRequest Id:
                                //If the StockInfoResponse is empty, we can use the Id to see what article code was requested from the output request we still have on the record.
                                message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + id + "[" + articleId.ToString() + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                "<Criteria ArticleId=\"" + articleId.ToString() + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\"/>" +
                                "</StockInfoRequest>" +
                                "</WWKS>";

                                //if (SharedVariables.IsPickupsOnlyChecked == true && articleId.Contains("RoWa"))
                                if (articleId.Contains("RoWa"))
                                {
                                    await SharedVariables.networkClient.SendAndReceiveAsync(message);
                                }
                                /*
                                if (SharedVariables.IsPickupsOnlyChecked == false)
                                {
                                    await SharedVariables.networkClient.SendAndReceiveAsync(message);
                                }*/

                            }
                            Debug.WriteLine($"ItemId {orderId} not found in StockOutputRecords");
                        }






                        /*
                        id = DateTime.UtcNow.ToString("HHmmssfff");

                        //Send a stockinforequest using the output ID from the specific article as StockInfoRequest Id:
                        //If the StockInfoResponse is empty, we can use the Id to see what article code was requested from the output request we still have on the record.
                        message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                        "<StockInfoRequest Id=\"" + id + "[" + articleId.ToString() + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                        "<Criteria ArticleId=\"" + articleId.ToString() + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\" />" +
                        "</StockInfoRequest>" +
                        "</WWKS>";

                        // Send the StockInfoRequest
                        await SharedVariables.networkClient.SendAndReceiveAsync(message);
                        // Handle the case where the itemId is not found in StockOutputRecords
                        Debug.WriteLine($"updating {articleId}...");
                        */
                    }

                    DataRow? dataRowToUpdate = dataTable.Rows.OfType<DataRow>().FirstOrDefault(row => row["Id"].ToString() == articleId);

                    if (dataRowToUpdate != null)
                    {
                        // Update the DataRow based on the status
                        int currentQuantity = Convert.ToInt32(dataRowToUpdate["Quantity"]);
                        int newQuantity = status == "Completed" ? currentQuantity - quantity : currentQuantity;

                        dataRowToUpdate["Quantity"] = newQuantity;
                        // Refresh the ListView with a timer
                        //PrepareRefresh();
                    }
                }
            });
        }


        private async void SearchPhrase_TextChanged(object sender, TextChangedEventArgs e)
        {
            // Cancel any previous task that was scheduled
            typingDelayCancellationTokenSource.Cancel();
            typingDelayCancellationTokenSource = new CancellationTokenSource();

            try
            {
                // Wait for the delay
                await Task.Delay(Convert.ToInt32(SharedVariables.ReadSpeed), typingDelayCancellationTokenSource.Token);

                // Pro
                if (SearchPhrase.Text.Length > 1)
                {
                    string searchTerm = SearchPhrase.Text;

                    //if (SharedVariables.IsPickupsOnlyChecked)
                    //{
                        searchTerm = "RoWa" + SearchPhrase.Text;
                        //Send StockInforequest if we are opting for a scan output...
                        if (SharedVariables.ScanOutput)
                        {
                            string id = DateTime.UtcNow.ToString("HHmmssfff");

                            string message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + id + "[" + searchTerm + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                    "<Criteria ArticleId=\"" + searchTerm + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\"/>" +
                                "</StockInfoRequest>" +
                            "</WWKS>";
                            // Send the StockInfoRequest
                            await SharedVariables.networkClient.SendAndReceiveAsync(message);
                            //return;
                        }
                    //}
                    if (dataTable != null && dataTable.Rows.Count > 0)
                    {
                        DataRow[] searchResults = dataTable.Select($"Id LIKE '%{searchTerm}%' OR Name LIKE '%{searchTerm}%' OR DosageForm LIKE '%{searchTerm}%' OR PackagingUnit LIKE '%{searchTerm}%'");

                        ObservableCollection<Article> displayedArticles = new ObservableCollection<Article>(
                                searchResults.Cast<DataRow>().Select(row =>
                                {
                                    string articleId = row["Id"]?.ToString() ?? string.Empty;
                                    int quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty);

                                    // Find a matching record in StockOutputRecords by articleId
                                    var matchingRecord = StockOutputRecords.FirstOrDefault(record => record.Item2 == articleId);

                                    // Determine the button color
                                    Microsoft.Maui.Graphics.Color buttonColor;
                                    if (matchingRecord != null)
                                    {
                                        // If there's a matching record, use its color
                                        buttonColor = matchingRecord.Item4;
                                    }
                                    else
                                    {
                                        // Otherwise, use the original logic based on quantity
                                        buttonColor = quantity == 0 ? Colors.Grey : Colors.Purple;
                                    }

                                    return new Article
                                    {
                                        Id = articleId,
                                        Name = row["Name"]?.ToString() ?? string.Empty,
                                        DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                                        PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                                        Quantity = quantity,
                                        ButtonColor = buttonColor
                                    };
                                })
                        );
                        // Bind the search results to myListView
                        if (displayedArticles.Count == 0)
                        {
                            string id = DateTime.UtcNow.ToString("HHmmssfff");

                            string message = "<WWKS Version=\"2.0\" TimeStamp=\"" + DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") + "\">" +
                                "<StockInfoRequest Id=\"" + id + "[" + searchTerm + "]" + "\" Source=\"" + SharedVariables.SourceNumber.ToString() + "\" Destination=\"999\" IncludePacks=\"False\" IncludeArticleDetails=\"True\">" +
                                    "<Criteria ArticleId=\"" + searchTerm + "\" StockLocationId=\"" + SharedVariables.RobotStockLocation + "\"/>" +
                                "</StockInfoRequest>" +
                            "</WWKS>";
                            // Send the StockInfoRequest
                            await SharedVariables.networkClient.SendAndReceiveAsync(message);
                        }
                        myConnectionView.ItemsSource = displayedArticles;
                        // Refresh the ListView
                        //ForceRefresh();
                        if (SharedVariables.ScanOutput && displayedArticles.Count == 1)
                        {
                            SendOutputOrder(displayedArticles[0]); // Placeholder method, replace with actual implementation
                        }

                    }

                }
                else
                {
                    //if (SharedVariables.IsPickupsOnlyChecked)
                    //{
                        string searchTerm = "RoWa";
                        DataRow[] searchResults = dataTable.Select($"Id LIKE '%{searchTerm}%' OR Name LIKE '%{searchTerm}%' OR DosageForm LIKE '%{searchTerm}%' OR PackagingUnit LIKE '%{searchTerm}%'");

                        ObservableCollection<Article> displayedArticles = new ObservableCollection<Article>(
                                searchResults.Cast<DataRow>().Select(row =>
                                {
                                    string articleId = row["Id"]?.ToString() ?? string.Empty;
                                    int quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty);

                                    // Find a matching record in StockOutputRecords by articleId
                                    var matchingRecord = StockOutputRecords.FirstOrDefault(record => record.Item2 == articleId);

                                    // Determine the button color
                                    Microsoft.Maui.Graphics.Color buttonColor;
                                    if (matchingRecord != null)
                                    {
                                        // If there's a matching record, use its color
                                        buttonColor = matchingRecord.Item4;
                                    }
                                    else
                                    {
                                        // Otherwise, use the original logic based on quantity
                                        buttonColor = quantity == 0 ? Colors.Grey : Colors.Purple;
                                    }

                                    return new Article
                                    {
                                        Id = articleId,
                                        Name = row["Name"]?.ToString() ?? string.Empty,
                                        DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                                        PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                                        Quantity = quantity,
                                        ButtonColor = buttonColor
                                    };
                                })

                        );

                        // Bind the search results to myListView
                        myConnectionView.ItemsSource = displayedArticles;
                        // Refresh the ListView
                        //ForceRefresh();
                    /*}
                    else
                    {
                        myConnectionView.ItemsSource = null;
                        ObservableCollection<Article> displayedArticles = new ObservableCollection<Article>();

                        foreach (DataRow row in dataTable.Rows)
                        {
                            string articleId = row["Id"]?.ToString() ?? string.Empty;
                            int quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty);

                            // Find a matching record in StockOutputRecords by articleId
                            var matchingRecord = StockOutputRecords.FirstOrDefault(record => record.Item2 == articleId);

                            // Determine the button color
                            Microsoft.Maui.Graphics.Color buttonColor;
                            if (matchingRecord != null)
                            {
                                // If there's a matching record, use its color
                                buttonColor = matchingRecord.Item4;
                            }
                            else
                            {
                                // Otherwise, use the original logic based on quantity
                                buttonColor = quantity == 0 ? Colors.Grey : Colors.Purple;
                            }
                            Article article = new Article
                            {
                                Id = row["Id"]?.ToString() ?? string.Empty,
                                Name = row["Name"]?.ToString() ?? string.Empty,
                                DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                                PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                                Quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty),
                                //MaxSubItemQuantity = int.Parse(row["MaxSubItemQuantity"].ToString())
                                ButtonColor = buttonColor
                            };

                            displayedArticles.Add(article);
                        }
                        myConnectionView.ItemsSource = displayedArticles;
                        // Refresh the ListView
                        //ForceRefresh();
                    }*/
                }

            }
            catch (TaskCanceledException)
            {
                // If a new character is typed before the delay is over, this catch block will be hit
                // Just ignore it, as it means a new delay has started
            }
        }

        private async void SendOutputOrder(Article article)
        {
            DateTime now = DateTime.Now;

            // Access properties of the Article class
            string articleId = article.Id;
            int articleQty = article.Quantity;
            var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item2.ToString() == articleId);

            string id = DateTime.UtcNow.ToString("HHmmssfff");
            if (existingItem == null)
            {
                // ComboBoxItem selectedPrioItem = (ComboBoxItem)PrioComboBox.SelectedItem;
                // Uncomment the line above if we need the priority from PrioComboBox
                // string priorityValue = selectedPrioItem.Content.ToString();
                string priorityValue = SharedVariables.SelectedPrioItemText; // You can replace this with the actual logic to get the priority

                SendOutputRequest(articleId, articleQty.ToString());
                await MainThread.InvokeOnMainThreadAsync(async () =>
                {
                    ChangeRowProperties(articleId, Colors.Blue, false);
                });

            }
            else
            {
                string existingOrderId = existingItem.Item1.ToString();
                SendTaskInfoRequest(existingOrderId);
            }
        }

        private async Task HandleTaskInfoResponseMessageAsync(object sender, MessageReceivedEventArgs e)
        {
            string taskInfoReponse = e.Message;
            string selectedPrioItem = SharedVariables.SelectedPrioItemText;
            Debug.WriteLine("TaskInfoResponse = " + taskInfoReponse);

            XDocument doc = XDocument.Parse(taskInfoReponse);

            // Extract OutputMessage element
            XElement? TaskInfoResponseMessage = doc.Root?.Elements("TaskInfoResponse").FirstOrDefault();

            if (TaskInfoResponseMessage != null)
            {
                string orderId = TaskInfoResponseMessage.Element("Task")?.Attribute("Id")?.Value ?? string.Empty;
                string status = TaskInfoResponseMessage.Element("Task")?.Attribute("Status")?.Value ?? string.Empty;
                var existingItem = StockOutputRecords.FirstOrDefault(item => item.Item1.ToString() == orderId);
                if (existingItem != null && (status == "Aborted" || status == "Unknown"))
                {
                    string articleId = existingItem.Item2;
                    StockOutputRecords.Remove(existingItem);
                    ChangeRowProperties(articleId, Colors.Purple, false);
                }
                if (existingItem != null && (status == "Incomplete"))
                {
                    string articleId = existingItem.Item2;
                    StockOutputRecords.Remove(existingItem);
                    ChangeRowProperties(articleId, Colors.Red, false);
                }
                if (existingItem != null && (status == "Queued"))
                {
                    string articleId = existingItem.Item2;
                    var index = StockOutputRecords.FindIndex(item => item.Item1.ToString() == orderId);
                    var updatedItem = Tuple.Create(existingItem.Item1, existingItem.Item2, existingItem.Item3, Colors.Blue);
                    StockOutputRecords[index] = updatedItem;
                    ChangeRowProperties(articleId, Colors.Blue, false);
                }
                if (existingItem != null && (status == "InProcess"))
                {
                    string articleId = existingItem.Item2;
                    var index = StockOutputRecords.FindIndex(item => item.Item1.ToString() == orderId);
                    var updatedItem = Tuple.Create(existingItem.Item1, existingItem.Item2, existingItem.Item3, Colors.Orange);
                    StockOutputRecords[index] = updatedItem;
                    ChangeRowProperties(articleId, Colors.Orange, false);
                }
                if (existingItem != null && (status == "Completed"))
                {
                    string articleId = existingItem.Item2;
                    StockOutputRecords.Remove(existingItem);
                    ChangeRowProperties(articleId, Colors.Green, true);
                }
            }
        }



        private void OnSearchClicked(object sender, EventArgs e)
        {
            try
            {
                string searchTerm = SearchPhrase.Text;
                if (searchTerm.Length > 2)
                {

                    DataRow[] searchResults = dataTable.Select($"Id LIKE '%{searchTerm}%' OR Name LIKE '%{searchTerm}%' OR DosageForm LIKE '%{searchTerm}%' OR PackagingUnit LIKE '%{searchTerm}%'");

                    //We have to figure out if any of the results are in the StockOutputRecords list, if so, take over the current button color

                    listViewArticles = searchResults.Cast<DataRow>().Select(row =>
                        new Article
                        {
                            Id = row["Id"]?.ToString() ?? string.Empty,
                            Name = row["Name"]?.ToString() ?? string.Empty,
                            DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                            PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                            Quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty),
                            //MaxSubItemQuantity = int.Parse(row["MaxSubItemQuantity"].ToString())
                            ButtonColor = int.Parse(row["Quantity"]?.ToString() ?? string.Empty) == 0 ? Colors.Grey : Colors.Purple
                        }).ToList();

                    // Bind the search results to myListView
                    myConnectionView.ItemsSource = listViewArticles;
                    // Refresh the ListView
                    //ForceRefresh();
                }
                else
                {
                    myConnectionView.ItemsSource = null;
                    listViewArticles = new List<Article>();

                    foreach (DataRow row in dataTable.Rows)
                    {
                        Article article = new Article
                        {
                            Id = row["Id"]?.ToString() ?? string.Empty,
                            Name = row["Name"]?.ToString() ?? string.Empty,
                            DosageForm = row["DosageForm"]?.ToString() ?? string.Empty,
                            PackagingUnit = row["PackagingUnit"]?.ToString() ?? string.Empty,
                            Quantity = int.Parse(row["Quantity"]?.ToString() ?? string.Empty),
                            //MaxSubItemQuantity = int.Parse(row["MaxSubItemQuantity"].ToString())
                            ButtonColor = int.Parse(row["Quantity"]?.ToString() ?? string.Empty) == 0 ? Colors.Grey : Colors.Purple
                        };

                        listViewArticles.Add(article);
                    }

                    myConnectionView.ItemsSource = listViewArticles;
                    // Refresh the ListView
                    //ForceRefresh();
                }
            }
            catch (Exception ex)
            {
                // Handle exceptions, you might want to log the exception details
                Debug.WriteLine($"An error occurred: {ex.Message}");
            }
        }

        protected override async void OnAppearing()
        {
            base.OnAppearing();
            await SettingsLoader.LoadSettingsAsync();
            if (ConnectivityState() != "none")
            {
                //SharedVariables.networkClient = new NetworkClient(this);
                if (string.IsNullOrEmpty(SharedVariables.ClientIpAddress))
                {
                    await SettingsLoader.LoadSettingsAsync();
                    if (!SharedVariables.networkClient.tcpClient.Connected)
                    {
                        await SharedVariables.networkClient.ConnectAsync(SharedVariables.ClientIpAddress, SharedVariables.ClientPort);
                    }
                    await SendHelloAsync();
                }
                else
                {
                    if (!SharedVariables.networkClient.tcpClient.Connected)
                    {
                        await SharedVariables.networkClient.ConnectAsync(SharedVariables.ClientIpAddress, SharedVariables.ClientPort);
                    }
                    await SendHelloAsync();
                }
            }

            timer = new System.Timers.Timer(3000); // Interval in milliseconds
            timer.Elapsed += OnTimedEvent;
            timer.AutoReset = true;
            timer.Enabled = true;

            //WeakReferenceMessenger.Default.Send(new UpdateStateLabel("Voorraad vernieuwen..."+ "[" + ConnectivityState() + "]"));

        }
        private async void OnTimedEvent(object sender, ElapsedEventArgs e)
        {
            // Function to check if the client connection is still active
            if (IsConnectionActive())
            {
                await MainThread.InvokeOnMainThreadAsync(() =>
                {
                    LBConnectionState.Text = "Verbonden " + RobotState;
                });
                // Other logic...
            }
            else
            {
                await MainThread.InvokeOnMainThreadAsync(() =>
                {
                    LBConnectionState.Text = "Niet verbonden " + RobotState;
                });
                Debug.WriteLine("Connection is not active. Attempting recovery...");

                // Attempt to recover the connection
                if (await RecoverConnection())
                {
                    await MainThread.InvokeOnMainThreadAsync(() =>
                    {
                        LBConnectionState.Text = "Verbonden " + RobotState;
                    });
                    Debug.WriteLine("Connection recovered");

                    // Send any necessary initialization messages
                    await SendHelloAsync();
                }
                else
                {
                    Debug.WriteLine("Connection recovery failed");
                }
            }
        }
        private bool IsConnectionActive()
        {
            if (SharedVariables.networkClient.tcpClient.Connected)
            {
                try
                {
                    // Try sending a small piece of data to check if the connection is still active
                    SharedVariables.networkClient.tcpClient.GetStream().Write(new byte[1], 0, 1);
                    return true;
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Connection is not active: " + ex.Message);
                    return false;
                }
            }
            else
            {
                return false;
            }
        }


        private async Task<bool> RecoverConnection()
        {

            if (SharedVariables.networkClient.IsValidIpAddress(SharedVariables.ClientIpAddress) && SharedVariables.networkClient.IsValidPort(SharedVariables.ClientPort.ToString()))
            {
                try
                {
                    // Close the existing connection (if any)
                    SharedVariables.networkClient.Close();

                    // Attempt to establish a new connection
                    await SharedVariables.networkClient.ConnectAsync(SharedVariables.ClientIpAddress, SharedVariables.ClientPort);

                    return SharedVariables.networkClient.tcpClient.Connected;
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Error during connection recovery: " + ex.Message);
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }

}

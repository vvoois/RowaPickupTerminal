using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using static RowaPickupMAUI.XmlDefinitions;
using System.Xml.Serialization;

namespace RowaPickupMAUI
{
    public class NetworkClient
    {
        public TcpClient tcpClient = new TcpClient();
        private Stream stream;
        private StreamReader reader;
        private StreamWriter writer;
        //private MainPage _mainPage;
        public event EventHandler<MessageReceivedEventArgs> MessageReceived;

        public NetworkClient()
        {
            //_mainPage = mainPage;
            tcpClient = SharedVariables.SharedTcpClient ?? new TcpClient();
        }

        // Connect to the server
        public async Task ConnectAsync(string serverIp, int port)
        {
            try
            {
                await tcpClient.ConnectAsync(serverIp, port);

                // Adjust buffer sizes based on your requirements
                tcpClient.ReceiveBufferSize = 8192; // Set the size you find appropriate
                tcpClient.SendBufferSize = 8192;    // Set the size you find appropriate

                stream = tcpClient.GetStream();
                reader = new StreamReader(stream, Encoding.UTF8, true, 8192);  // Adjust buffer size here
                writer = new StreamWriter(stream, new UTF8Encoding(false)) { AutoFlush = true };
                _ = Task.Run(async () => await ReceiveMessagesAsync());
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Error connecting to server: {ex}");
            }
        }

        protected virtual void OnMessageReceived(MessageReceivedEventArgs e)
        {
            MessageReceived?.Invoke(this, e);
        }


        public async Task SendAndReceiveAsync(string message)
        {
            message = message.TrimStart('\uFEFF', '\u200B');
            await SendMessageAsync(message);
        }

        // Send a message to the server
        public async Task SendMessageAsync(string message)
        {
            try
            {
                // Filter out illegal characters from the XML message
                message = RemoveIllegalCharacters(message);

                // If the resulting message is not empty, send it
                if (!string.IsNullOrEmpty(message))
                {
                    await writer.WriteLineAsync(message);
                }
            }
            catch (Exception ex)
            {
                // Handle sending message error
                Debug.WriteLine($"Error sending message: {ex.Message}");
            }
        }

        private string RemoveIllegalCharacters(string input)
        {
            // Define a regular expression pattern to match illegal characters
            string pattern = @"[^\u0009\u000A\u000D\u0020-\uD7FF\uE000-\uFFFD\u10000-\u10FFFF]";

            // Use Regex.Replace to remove or replace illegal characters
            string result = Regex.Replace(input, pattern, "");

            return result;
        }

        // Receive messages from the server
        public async Task ReceiveMessagesAsync()
        {
            char[] buffer = new char[8192]; // Adjust buffer size based on your needs
            StringBuilder messageBuilder = new StringBuilder();
            string endTag = "</WWKS>";

            while (tcpClient.Connected) // Check if TcpClient is connected
            {
                try
                {
                    int bytesRead = await reader.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead == 0)
                        break;

                    messageBuilder.Append(buffer, 0, bytesRead);

                    // Check if the end tag is present in the received data
                    string message = messageBuilder.ToString();
                    if (message.Contains(endTag))
                    {
                        // If end tag found, trigger the handlers for the complete message
                        await HandleReceivedMessageAsync(message);
                        // Clear the messageBuilder for the next message
                        messageBuilder.Clear();
                    }
                    else
                    {
                        // If end tag not found, continue reading until it's complete
                        continue;
                    }
                }
                catch (IOException)
                {
                    // An IOException may occur if the connection is closed unexpectedly
                    // Handle the situation here, perhaps by attempting to reconnect
                    Console.WriteLine("Connection closed unexpectedly. Attempting to reconnect...");
                    // You may call your reconnect logic here
                }
            }

            // Handle the case where the TcpClient is no longer connected
            Console.WriteLine("TcpClient is not connected. Handle the situation accordingly.");
            // You may call your reconnect logic here
        }


        public async Task HandleReceivedMessageAsync(string completeMessage)
        {
            // Find the message type in the response
            string messageType = await Task.Run(() => GetMessageResponseType(completeMessage));
            /*
            // Use the message dispatcher to handle the response
            if (_mainPage.messageHandlers.TryGetValue(messageType, out var handler))
            {
                await handler(completeMessage);
            }
            */
            // Raise the event with the message
            OnMessageReceived(new MessageReceivedEventArgs(messageType, completeMessage));
        }

        public string GetMessageResponseType(string wwksMessage)
        {
            try
            {
                var serializer = new XmlSerializer(typeof(XmlWrapper));

                using (var reader = new StringReader(wwksMessage))
                {
                    var wrapper = (XmlWrapper)serializer.Deserialize(reader);
                    if (wrapper != null)
                    {
                        // Determine the message type based on the actual type of the deserialized object
                        if (wrapper.HelloResponse != null)
                        {
                            return "HelloResponse";
                        }
                        else if (wrapper.StatusResponse != null)
                        {
                            return "StatusResponse";
                        }
                        else if (wrapper.StockInfoResponse != null)
                        {
                            return "StockInfoResponse";
                        }
                        else if (wrapper.OutputResponse != null)
                        {
                            return "OutputResponse";
                        }
                        else if (wrapper.OutputMessage != null)
                        {
                            return "OutputMessage";
                        }
                        else if (wrapper.InputMessage != null)
                        {
                            return "InputMessage";
                        }
                        else if (wrapper.TaskInfoResponse != null)
                        {
                            return "TaskInfoResponse";
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Error extracting message type: {ex}");
            }

            return string.Empty;
        }

        // Close the connection
        public void Close()
        {
            reader?.Dispose();
            writer?.Dispose();
            tcpClient?.Close();
            // Recreate the TcpClient
            tcpClient = new TcpClient();
        }
        public bool IsValidIpAddress(string ipAddress)
        {
            // Add your validation logic for IP address (if needed)
            // For a basic check, you can use IPAddress.TryParse
            return IPAddress.TryParse(ipAddress, out _);
        }

        public bool IsValidPort(string port)
        {
            // Add your validation logic for port (if needed)
            // For a basic check, you can use int.TryParse and check if it's within valid port range
            if (int.TryParse(port, out int portNumber))
            {
                return portNumber > 0 && portNumber <= 65535;
            }
            return false;
        }
    }
}
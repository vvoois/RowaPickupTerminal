using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace RowaPickupMAUI
{
    public class XmlDefinitions
    {
        [XmlInclude(typeof(StockInfoResponse))]
        [XmlInclude(typeof(InputMessage))]
        [XmlInclude(typeof(HelloResponse))]
        [XmlInclude(typeof(StatusResponse))]
        [XmlInclude(typeof(OutputResponse))]
        [XmlInclude(typeof(OutputMessage))]
        [XmlRoot("WWKS")]
        public class XmlWrapper
        {
            [XmlAttribute("Version")]
            public string Version { get; set; } = "";

            [XmlAttribute("TimeStamp")]
            public DateTime TimeStamp { get; set; } = DateTime.Now;

            [XmlElement("HelloResponse")]
            public HelloResponse? HelloResponse { get; set; }
            [XmlElement("StatusResponse")]
            public StatusResponse? StatusResponse { get; set; }
            [XmlElement("StockInfoResponse")]
            public StockInfoResponse? StockInfoResponse { get; set; }
            [XmlElement("OutputResponse")]
            public OutputResponse? OutputResponse { get; set; }
            [XmlElement("OutputMessage")]
            public OutputResponse? OutputMessage { get; set; }
            [XmlElement("InputMessage")]
            public InputMessage? InputMessage { get; set; }
            [XmlElement("TaskInfoResponse")]
            public TaskInfoResponse? TaskInfoResponse { get; set; }
        }

        public class BaseMessage
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = "1001";

            [XmlAttribute("Source")]
            public string Source { get; set; } = "100";

            [XmlAttribute("Destination")]
            public string Destination { get; set; } = "999";
        }

        public class StatusResponse : BaseMessage
        {
            [XmlElement("StatusResponse")]
            public StatusResponseDetails? Details { get; set; }
        }

        public class StatusResponseDetails
        {
            [XmlAttribute("State")]
            public string State { get; set; } = string.Empty;

            [XmlElement("Component")]
            public List<ComponentStatus>? Components { get; set; }
        }

        public class ComponentStatus
        {
            [XmlAttribute("Type")]
            public string Type { get; set; } = string.Empty;

            [XmlAttribute("Description")]
            public string Description { get; set; } = string.Empty;

            [XmlAttribute("State")]
            public string State { get; set; } = string.Empty;

            [XmlAttribute("StateText")]
            public string StateText { get; set; } = string.Empty;
        }

        [XmlRoot("StockInfoResponse")]
        public class StockInfoResponse : BaseMessage
        {
            [XmlElement("Article")]
            public List<Article>? Articles { get; set; }
        }
        public class Article : INotifyPropertyChanged
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Name")]
            public string Name { get; set; } = string.Empty;

            [XmlAttribute("DosageForm")]
            public string DosageForm { get; set; } = string.Empty;

            [XmlAttribute("PackagingUnit")]
            public string PackagingUnit { get; set; } = string.Empty;

            [XmlAttribute("Quantity")]
            public int Quantity { get; set; } = 0;

            [XmlAttribute("MaxSubItemQuantity")]
            public int MaxSubItemQuantity { get; set; } = 0;

            [XmlElement("Pack")]
            public Pack? Pack { get; set; }

            private Color _buttonColor = Colors.Purple; // Default color
            public Color ButtonColor
            {
                get => _buttonColor;
                set
                {
                    if (_buttonColor != value)
                    {
                        _buttonColor = value;
                        OnPropertyChanged(nameof(ButtonColor));
                    }
                }
            }
            public event PropertyChangedEventHandler? PropertyChanged;

            protected virtual void OnPropertyChanged(string propertyName)
            {
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        [XmlRoot("HelloResponse")]
        public class HelloResponse : BaseMessage
        {

            [XmlElement("Subscriber")]
            public Subscriber? Subscriber { get; set; } 
        }

        public class Subscriber
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Type")]
            public string Type { get; set; } = string.Empty;

            [XmlAttribute("Manufacturer")]
            public string Manufacturer { get; set; } = string.Empty;

            [XmlAttribute("ProductInfo")]
            public string ProductInfo { get; set; } = string.Empty;

            [XmlAttribute("VersionInfo")]
            public string VersionInfo { get; set; } = string.Empty;

            [XmlElement("Capability")]
            public List<Capability>? Capabilities { get; set; } 
        }

        public class Capability
        {
            [XmlAttribute("Name")]
            public string Name { get; set; } = string.Empty;
        }

        [XmlRoot("InputMessage")]
        public class InputMessage : BaseMessage
        {
            [XmlElement("Article")]
            public Article? Article { get; set; }
        }



        public class Pack
        {
            [XmlAttribute("Index")]
            public int Index { get; set; } = 0;

            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("BatchNumber")]
            public string BatchNumber { get; set; } = string.Empty;

            [XmlAttribute("ExternalId")]
            public string ExternalId { get; set; } = string.Empty;

            [XmlAttribute("ExpiryDate")]
            public DateTime ExpiryDate { get; set; }=DateTime.Now;

            [XmlAttribute("Depth")]
            public int Depth { get; set; } = 0;

            [XmlAttribute("Width")]
            public int Width { get; set; } = 0;

            [XmlAttribute("Height")]
            public int Height { get; set; } = 0;

            [XmlAttribute("Shape")]
            public string Shape { get; set; } = string.Empty;

            [XmlAttribute("State")]
            public string State { get; set; } = string.Empty;

            [XmlElement("Handling")]
            public Handling? Handling { get; set; }
        }

        public class Handling
        {
            [XmlAttribute("Input")]
            public string Input { get; set; } = string.Empty;

            [XmlAttribute("Text")]
            public string Text { get; set; } = string.Empty;
        }
        public class OutputResponse
        {
            [XmlElement("OutputResponse")]
            public OutputResponseDetails? Details { get; set; }
        }

        public class OutputResponseDetails
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Source")]
            public int Source { get; set; } = 100;

            [XmlAttribute("Destination")]
            public int Destination { get; set; } = 999;

            [XmlElement("BoxNumber")]
            public string BoxNumber { get; set; } = string.Empty;

            [XmlElement("Priority")]
            public string Priority { get; set; } = string.Empty;

            [XmlElement("OutputDestination")]
            public int OutputDestination { get; set; } = 1;

            [XmlElement("OutputPoint")]
            public int? OutputPoint { get; set; } = 0;

            [XmlElement("Status")]
            public string Status { get; set; } = string.Empty;

            [XmlElement("Details")]
            public OutputDetails? DetailsElement { get; set; }

            [XmlElement("Criteria")]
            public List<OutputCriteria>? Criteria { get; set; }
        }

        public class OutputCriteria
        {
            [XmlAttribute("ArticleId")]
            public string ArticleId { get; set; } = string.Empty;

            [XmlAttribute("Quantity")]
            public int Quantity { get; set; }=0;

            [XmlAttribute("SubItemQuantity")]
            public string SubItemQuantity { get; set; } = string.Empty;

            [XmlAttribute("MinimumExpiryDate")]
            public DateTime MinimumExpiryDate { get; set; }=DateTime.Now;

            [XmlElement("Label")]
            public List<Label>? Label { get; set; }
        }

        public class Label
        {
            [XmlAttribute("TemplateId")]
            public string TemplateId { get; set; } = string.Empty;
            [XmlElement("Content")]
            public string ContentData { get; set; } = string.Empty;
        }


        public class OutputMessage
        {
            [XmlElement("OutputMessage")]
            public OutputMessageDetails? Details { get; set; }
        }

        public class OutputMessageDetails
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Source")]
            public int Source { get; set; } = 100;

            [XmlAttribute("Destination")]
            public int Destination { get; set; } = 999;

            [XmlElement("Details")]
            public OutputDetails? DetailsElement { get; set; }

            [XmlElement("Article")]
            public List<ArticleOutput>? Articles { get; set; }
        }

        public class OutputDetails
        {
            [XmlAttribute("Priority")]
            public string Priority { get; set; } = string.Empty;

            [XmlAttribute("OutputDestination")]
            public int OutputDestination { get; set; }=1;

            [XmlAttribute("OutputPoint")]
            public int OutputPoint { get; set; } = 0;

            [XmlAttribute("Status")]
            public string Status { get; set; } = string.Empty;
        }

        public class ArticleOutput
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("VirtualId")]
            public string VirtualId { get; set; } = string.Empty;

            [XmlElement("Pack", IsNullable = true)]
            public PackOutput? Pack { get; set; }
        }

        public class PackOutput
        {
            [XmlAttribute("Id")]
            public int Id { get; set; } = 0;

            [XmlAttribute("BatchNumber")]
            public string BatchNumber { get; set; } = string.Empty;

            [XmlAttribute("ExternalId")]
            public string ExternalId { get; set; } = string.Empty;

            [XmlAttribute("ExpiryDate")]
            public DateTime ExpiryDate { get; set; }=DateTime.Now;

            [XmlAttribute("Depth")]
            public int Depth { get; set; } = 0;

            [XmlAttribute("Width")]
            public int Width { get; set; } = 0;

            [XmlAttribute("Height")]
            public int Height { get; set; } = 0;

            [XmlAttribute("Shape")]
            public string Shape { get; set; } = string.Empty;

            [XmlAttribute("IsInFridge")]
            public bool IsInFridge { get; set; } = false;

            [XmlAttribute("OutputDestination")]
            public int OutputDestination { get; set; } = 1;

            [XmlAttribute("LabelStatus")]
            public string LabelStatus { get; set; } = string.Empty;
        }

        public class OutputInfoRequest
        {
            [XmlElement("OutputInfoRequest")]
            public OutputInfoRequestDetails? Details { get; set; }
        }

        public class OutputInfoRequestDetails
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Source")]
            public int Source { get; set; } = 100;

            [XmlAttribute("Destination")]
            public int Destination { get; set; } = 999;

            [XmlElement("IncludeTaskDetails")]
            public bool IncludeTaskDetails { get; set; } = false;

            [XmlElement("Task")]
            public TaskDetails? Task { get; set; }
        }
        /*<WWKS Version="2.0" TimeStamp="2013-04-16T11:14:00Z"> <TaskInfoRequest Id="3330" Source="100" Destination="999"> <Task Type="Output" Id="1004"/> </TaskInfoRequest> </WWKS> */
        public class TaskDetails
        {
            [XmlElement("Type")]
            public string Type { get; set; } = "Output"; //or StockDelivery
        }
        public class TaskInfoRequest
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Source")]
            public int Source { get; set; } = 100;

            [XmlAttribute("Destination")]
            public int Destination { get; set; } = 999;

            [XmlAttribute("IncludeTaskDetails")]
            public bool IncludeTaskDetails { get; set; }= false;

            [XmlElement("Task")]
            public TaskInfoRequestDetails? Task { get; set; }
        }

        public class TaskInfoRequestDetails
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlElement("Type")]
            public string Type { get; set; } = "Output"; //or StockDelivery
        }
        /* 
<WWKS Version="2.0" TimeStamp="2013-04-16T11:14:00Z">
	<TaskInfoResponse Id="3330" Source="999" Destination="100">
		<Task Type="Output" Id="1004" Status="Completed">
			<Article Id="0004-56-034-G00025T">
				<Pack Id="5637" BatchNumber="Omepra0004" ExternalId="PalH09051200001" ExpiryDate="2015-11-05" Depth="50" Width="50" Height="50" Shape="Cuboid" OutputDestination="3" LabelStatus="Labelled" BoxNumber="123"/>
			</Article>
			<Box Number="123"/>
		</Task>
	</TaskInfoResponse>
</WWKS>
        */
        public class TaskInfoResponse
        {
            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Source")]
            public int Source { get; set; } = 100;

            [XmlAttribute("Destination")]
            public int Destination { get; set; } = 999;

            [XmlElement("Task")]
            public TaskInfoResponseDetails? Task { get; set; }
        }
        public class TaskInfoResponseDetails
        {
            [XmlAttribute("Type")]
            public string Type { get; set; } = string.Empty;

            [XmlAttribute("Id")]
            public string Id { get; set; } = string.Empty;

            [XmlAttribute("Status")]
            public string Status { get; set; } = string.Empty;

            [XmlElement("Article")]
            public List<Article>? Articles { get; set; }
            [XmlElement("Box")]
            public BoxDetails? Box { get; set; }
        }
        public class BoxDetails
        {
            [XmlElement("Box")]
            public int Number { get; set; } = 0;
        }
    }
}

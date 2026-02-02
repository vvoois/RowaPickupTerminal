// XmlDefinitions.cpp
// Minimal PugiXML-backed C++ translation of the C# XmlDefinitions types.
// Provides simple structs and load/save helpers for the XML structure.
// Designed to be copy/pasted and compiled with pugi (https://pugixml.org/)
//
// Notes:
// - Date/time fields are represented as std::string (ISO format) to keep parsing simple.
// - Collections use std::vector.
// - Each struct exposes `bool load(const pugi::xml_node&)` and `pugi::xml_node save(pugi::xml_node&) const`.
// - This file is self-contained; include pugi.hpp in your project include path.

#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include "pugixml.hpp"

namespace RowaPickupSlim::XmlDefinitions
{
    using std::string;
    using std::vector;
    using std::optional;

    // Forward declarations
    struct ComponentStatus;
    struct StatusResponseDetails;
    struct StatusResponse;
    struct Pack;
    struct Handling;
    struct Article;
    struct StockInfoResponse;
    struct Subscriber;
    struct HelloResponse;
    struct Capability;
    struct InputMessage;
    struct BaseMessage;
    struct OutputDetails;
    struct OutputCriteria;
    struct Label;
    struct OutputResponseDetails;
    struct OutputResponse;
    struct PackOutput;
    struct ArticleOutput;
    struct OutputMessageDetails;
    struct OutputMessage;
    struct TaskInfoRequestDetails;
    struct TaskInfoRequest;
    struct TaskDetails;
    struct TaskInfoResponseDetails;
    struct TaskInfoResponse;
    struct BoxDetails;

    // Utility helpers
    static inline string get_attr(const pugi::xml_node& n, const char* name, const char* def = "")
    {
        auto attr = n.attribute(name);
        return attr ? attr.as_string() : def;
    }
    static inline int get_attr_int(const pugi::xml_node& n, const char* name, int def = 0)
    {
        auto attr = n.attribute(name);
        return attr ? attr.as_int() : def;
    }
    static inline bool get_attr_bool(const pugi::xml_node& n, const char* name, bool def = false)
    {
        auto attr = n.attribute(name);
        return attr ? attr.as_bool() : def;
    }

    // BaseMessage (attributes Id, Source, Destination)
    struct BaseMessage
    {
        string Id = "1001";
        string Source = "100";
        string Destination = "999";

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id", Id.c_str());
            Source = get_attr(n, "Source", Source.c_str());
            Destination = get_attr(n, "Destination", Destination.c_str());
            return true;
        }

        pugi::xml_node save(pugi::xml_node& n) const
        {
            n.append_attribute("Id") = Id.c_str();
            n.append_attribute("Source") = Source.c_str();
            n.append_attribute("Destination") = Destination.c_str();
            return n;
        }
    };

    // ComponentStatus
    struct ComponentStatus
    {
        string Type;
        string Description;
        string State;
        string StateText;

        bool load(const pugi::xml_node& n)
        {
            Type = get_attr(n, "Type");
            Description = get_attr(n, "Description");
            State = get_attr(n, "State");
            StateText = get_attr(n, "StateText");
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Component");
            node.append_attribute("Type") = Type.c_str();
            node.append_attribute("Description") = Description.c_str();
            node.append_attribute("State") = State.c_str();
            node.append_attribute("StateText") = StateText.c_str();
            return node;
        }
    };

    // StatusResponseDetails
    struct StatusResponseDetails
    {
        string State;
        vector<ComponentStatus> Components;

        bool load(const pugi::xml_node& n)
        {
            State = get_attr(n, "State");
            Components.clear();
            for (auto c : n.children("Component"))
            {
                ComponentStatus cs;
                cs.load(c);
                Components.push_back(std::move(cs));
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("StatusResponse");
            node.append_attribute("State") = State.c_str();
            for (auto const& c : Components) c.save(node);
            return node;
        }
    };

    // StatusResponse (inherits BaseMessage)
    struct StatusResponse : BaseMessage
    {
        optional<StatusResponseDetails> Details;

        bool load(const pugi::xml_node& n)
        {
            BaseMessage::load(n);
            auto dnode = n.child("StatusResponse");
            if (dnode)
            {
                StatusResponseDetails d;
                d.load(dnode);
                Details = std::move(d);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("StatusResponse");
            BaseMessage::save(node);
            if (Details) Details->save(node);
            return node;
        }
    };

    // Handling
    struct Handling
    {
        string Input;
        string Text;

        bool load(const pugi::xml_node& n)
        {
            Input = get_attr(n, "Input");
            Text = get_attr(n, "Text");
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Handling");
            node.append_attribute("Input") = Input.c_str();
            node.append_attribute("Text") = Text.c_str();
            return node;
        }
    };

    // Pack
    struct Pack
    {
        int Index = 0;
        string Id;
        string BatchNumber;
        string ExternalId;
        string ExpiryDate; // ISO string
        int Depth = 0;
        int Width = 0;
        int Height = 0;
        string Shape;
        string State;
        optional<Handling> HandlingElement;

        bool load(const pugi::xml_node& n)
        {
            Index = get_attr_int(n, "Index", Index);
            Id = get_attr(n, "Id");
            BatchNumber = get_attr(n, "BatchNumber");
            ExternalId = get_attr(n, "ExternalId");
            ExpiryDate = get_attr(n, "ExpiryDate");
            Depth = get_attr_int(n, "Depth", Depth);
            Width = get_attr_int(n, "Width", Width);
            Height = get_attr_int(n, "Height", Height);
            Shape = get_attr(n, "Shape");
            State = get_attr(n, "State");
            auto h = n.child("Handling");
            if (h)
            {
                Handling hh;
                hh.load(h);
                HandlingElement = std::move(hh);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Pack");
            node.append_attribute("Index") = Index;
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("BatchNumber") = BatchNumber.c_str();
            node.append_attribute("ExternalId") = ExternalId.c_str();
            node.append_attribute("ExpiryDate") = ExpiryDate.c_str();
            node.append_attribute("Depth") = Depth;
            node.append_attribute("Width") = Width;
            node.append_attribute("Height") = Height;
            node.append_attribute("Shape") = Shape.c_str();
            node.append_attribute("State") = State.c_str();
            if (HandlingElement) HandlingElement->save(node);
            return node;
        }
    };

    // Article (Stock)
    struct Article
    {
        string Id;
        string Name;
        string DosageForm;
        string PackagingUnit;
        int Quantity = 0;
        int MaxSubItemQuantity = 0;
        optional<Pack> PackElement;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Name = get_attr(n, "Name");
            DosageForm = get_attr(n, "DosageForm");
            PackagingUnit = get_attr(n, "PackagingUnit");
            Quantity = get_attr_int(n, "Quantity", Quantity);
            MaxSubItemQuantity = get_attr_int(n, "MaxSubItemQuantity", MaxSubItemQuantity);
            auto p = n.child("Pack");
            if (p)
            {
                Pack pk;
                pk.load(p);
                PackElement = std::move(pk);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Article");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Name") = Name.c_str();
            node.append_attribute("DosageForm") = DosageForm.c_str();
            node.append_attribute("PackagingUnit") = PackagingUnit.c_str();
            node.append_attribute("Quantity") = Quantity;
            node.append_attribute("MaxSubItemQuantity") = MaxSubItemQuantity;
            if (PackElement) PackElement->save(node);
            return node;
        }
    };

    // StockInfoResponse
    struct StockInfoResponse : BaseMessage
    {
        vector<Article> Articles;

        bool load(const pugi::xml_node& n)
        {
            BaseMessage::load(n);
            Articles.clear();
            for (auto a : n.children("Article"))
            {
                Article art;
                art.load(a);
                Articles.push_back(std::move(art));
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("StockInfoResponse");
            BaseMessage::save(node);
            for (auto const& a : Articles) a.save(node);
            return node;
        }
    };

    // Capability
    struct Capability
    {
        string Name;
        bool load(const pugi::xml_node& n) { Name = get_attr(n, "Name"); return true; }
        pugi::xml_node save(pugi::xml_node& parent) const { auto node = parent.append_child("Capability"); node.append_attribute("Name") = Name.c_str(); return node; }
    };

    // Subscriber
    struct Subscriber
    {
        string Id;
        string Type;
        string Manufacturer;
        string ProductInfo;
        string VersionInfo;
        string TenantId;
        vector<Capability> Capabilities;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Type = get_attr(n, "Type");
            Manufacturer = get_attr(n, "Manufacturer");
            ProductInfo = get_attr(n, "ProductInfo");
            VersionInfo = get_attr(n, "VersionInfo");
            TenantId = "";
            Capabilities.clear();
            for (auto c : n.children("Capability"))
            {
                Capability cap; cap.load(c); Capabilities.push_back(std::move(cap));
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Subscriber");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Type") = Type.c_str();
            node.append_attribute("Manufacturer") = Manufacturer.c_str();
            node.append_attribute("ProductInfo") = ProductInfo.c_str();
            node.append_attribute("VersionInfo") = VersionInfo.c_str();
            for (auto const& c : Capabilities) c.save(node);
            return node;
        }
    };

    // HelloResponse
    struct HelloResponse : BaseMessage
    {
        optional<Subscriber> SubscriberElement;

        bool load(const pugi::xml_node& n)
        {
            BaseMessage::load(n);
            auto s = n.child("Subscriber");
            if (s)
            {
                Subscriber sb; sb.load(s); SubscriberElement = std::move(sb);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("HelloResponse");
            BaseMessage::save(node);
            if (SubscriberElement) SubscriberElement->save(node);
            return node;
        }
    };

    // InputMessage
    struct InputMessage : BaseMessage
    {
        optional<Article> ArticleElement;

        bool load(const pugi::xml_node& n)
        {
            BaseMessage::load(n);
            auto a = n.child("Article");
            if (a) { Article art; art.load(a); ArticleElement = std::move(art); }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("InputMessage");
            BaseMessage::save(node);
            if (ArticleElement) ArticleElement->save(node);
            return node;
        }
    };

    // OutputDetails (used in multiple contexts)
    struct OutputDetails
    {
        string Priority;
        int OutputDestination = 1;
        int OutputPoint = 0;
        string Status;

        bool load(const pugi::xml_node& n)
        {
            Priority = get_attr(n, "Priority");
            OutputDestination = get_attr_int(n, "OutputDestination", OutputDestination);
            OutputPoint = get_attr_int(n, "OutputPoint", OutputPoint);
            Status = get_attr(n, "Status");
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Details");
            node.append_attribute("Priority") = Priority.c_str();
            node.append_attribute("OutputDestination") = OutputDestination;
            node.append_attribute("OutputPoint") = OutputPoint;
            node.append_attribute("Status") = Status.c_str();
            return node;
        }
    };

    // Label
    struct Label
    {
        string TemplateId;
        string ContentData;

        bool load(const pugi::xml_node& n)
        {
            TemplateId = get_attr(n, "TemplateId");
            auto c = n.child("Content");
            ContentData = c ? c.text().as_string() : string();
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Label");
            node.append_attribute("TemplateId") = TemplateId.c_str();
            auto c = node.append_child("Content");
            c.text().set(ContentData.c_str());
            return node;
        }
    };

    // OutputCriteria
    struct OutputCriteria
    {
        string ArticleId;
        int Quantity = 0;
        string SubItemQuantity;
        string MinimumExpiryDate;
        vector<Label> Labels;

        bool load(const pugi::xml_node& n)
        {
            ArticleId = get_attr(n, "ArticleId");
            Quantity = get_attr_int(n, "Quantity", Quantity);
            SubItemQuantity = get_attr(n, "SubItemQuantity");
            MinimumExpiryDate = get_attr(n, "MinimumExpiryDate");
            Labels.clear();
            for (auto l : n.children("Label")) { Label lab; lab.load(l); Labels.push_back(std::move(lab)); }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Criteria");
            node.append_attribute("ArticleId") = ArticleId.c_str();
            node.append_attribute("Quantity") = Quantity;
            node.append_attribute("SubItemQuantity") = SubItemQuantity.c_str();
            node.append_attribute("MinimumExpiryDate") = MinimumExpiryDate.c_str();
            for (auto const& l : Labels) l.save(node);
            return node;
        }
    };

    // OutputResponseDetails
    struct OutputResponseDetails
    {
        string Id;
        int Source = 100;
        int Destination = 999;
        string BoxNumber;
        string Priority;
        int OutputDestination = 1;
        optional<int> OutputPoint;
        string Status;
        optional<OutputDetails> DetailsElement;
        vector<OutputCriteria> Criteria;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Source = get_attr_int(n, "Source", Source);
            Destination = get_attr_int(n, "Destination", Destination);
            BoxNumber = get_attr(n, "BoxNumber");
            Priority = get_attr(n, "Priority");
            OutputDestination = get_attr_int(n, "OutputDestination", OutputDestination);
            if (n.child("OutputPoint")) OutputPoint = get_attr_int(n, "OutputPoint", 0);
            Status = get_attr(n, "Status");
            auto det = n.child("Details");
            if (det) { OutputDetails d; d.load(det); DetailsElement = std::move(d); }
            Criteria.clear();
            for (auto c : n.children("Criteria"))
            {
                OutputCriteria oc; oc.load(c); Criteria.push_back(std::move(oc));
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("OutputResponse");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Source") = Source;
            node.append_attribute("Destination") = Destination;
            auto bn = node.append_child("BoxNumber"); bn.text().set(BoxNumber.c_str());
            auto pr = node.append_child("Priority"); pr.text().set(Priority.c_str());
            auto od = node.append_child("OutputDestination"); od.text().set(std::to_string(OutputDestination).c_str());
            if (OutputPoint) { auto op = node.append_child("OutputPoint"); op.text().set(std::to_string(*OutputPoint).c_str()); }
            auto st = node.append_child("Status"); st.text().set(Status.c_str());
            if (DetailsElement) DetailsElement->save(node);
            for (auto const& c : Criteria) c.save(node);
            return node;
        }
    };

    // OutputResponse wrapper
    struct OutputResponse
    {
        optional<OutputResponseDetails> Details;

        bool load(const pugi::xml_node& n)
        {
            auto d = n.child("OutputResponse");
            if (d)
            {
                OutputResponseDetails dd; dd.load(d); Details = std::move(dd);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("OutputResponse");
            if (Details) Details->save(node);
            return node;
        }
    };

    // PackOutput
    struct PackOutput
    {
        int Id = 0;
        string BatchNumber;
        string ExternalId;
        string ExpiryDate;
        int Depth = 0;
        int Width = 0;
        int Height = 0;
        string Shape;
        bool IsInFridge = false;
        int OutputDestination = 1;
        string LabelStatus;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr_int(n, "Id", Id);
            BatchNumber = get_attr(n, "BatchNumber");
            ExternalId = get_attr(n, "ExternalId");
            ExpiryDate = get_attr(n, "ExpiryDate");
            Depth = get_attr_int(n, "Depth", Depth);
            Width = get_attr_int(n, "Width", Width);
            Height = get_attr_int(n, "Height", Height);
            Shape = get_attr(n, "Shape");
            IsInFridge = get_attr_bool(n, "IsInFridge", IsInFridge);
            OutputDestination = get_attr_int(n, "OutputDestination", OutputDestination);
            LabelStatus = get_attr(n, "LabelStatus");
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Pack");
            node.append_attribute("Id") = Id;
            node.append_attribute("BatchNumber") = BatchNumber.c_str();
            node.append_attribute("ExternalId") = ExternalId.c_str();
            node.append_attribute("ExpiryDate") = ExpiryDate.c_str();
            node.append_attribute("Depth") = Depth;
            node.append_attribute("Width") = Width;
            node.append_attribute("Height") = Height;
            node.append_attribute("Shape") = Shape.c_str();
            node.append_attribute("IsInFridge") = IsInFridge;
            node.append_attribute("OutputDestination") = OutputDestination;
            node.append_attribute("LabelStatus") = LabelStatus.c_str();
            return node;
        }
    };

    // ArticleOutput
    struct ArticleOutput
    {
        string Id;
        string VirtualId;
        optional<PackOutput> Pack;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            VirtualId = get_attr(n, "VirtualId");
            auto p = n.child("Pack");
            if (p) { PackOutput po; po.load(p); Pack = std::move(po); }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Article");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("VirtualId") = VirtualId.c_str();
            if (Pack) Pack->save(node);
            return node;
        }
    };

    // OutputMessageDetails
    struct OutputMessageDetails
    {
        string Id;
        int Source = 100;
        int Destination = 999;
        optional<OutputDetails> DetailsElement;
        vector<ArticleOutput> Articles;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Source = get_attr_int(n, "Source", Source);
            Destination = get_attr_int(n, "Destination", Destination);
            auto det = n.child("Details");
            if (det) { OutputDetails d; d.load(det); DetailsElement = std::move(d); }
            Articles.clear();
            for (auto a : n.children("Article"))
            {
                ArticleOutput ao; ao.load(a); Articles.push_back(std::move(ao));
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("OutputMessage");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Source") = Source;
            node.append_attribute("Destination") = Destination;
            if (DetailsElement) DetailsElement->save(node);
            for (auto const& a : Articles) a.save(node);
            return node;
        }
    };

    struct OutputMessage
    {
        optional<OutputMessageDetails> Details;
        bool load(const pugi::xml_node& n)
        {
            auto d = n.child("OutputMessage");
            if (d) { OutputMessageDetails dd; dd.load(d); Details = std::move(dd); }
            return true;
        }
        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("OutputMessage");
            if (Details) Details->save(node);
            return node;
        }
    };

    // Task related structs
    struct TaskDetails
    {
        string Type = "Output";
        bool load(const pugi::xml_node& n)
        {
            // In C# this was an element with content; accept either attribute or text
            if (n.attribute("Type")) Type = get_attr(n, "Type");
            else Type = n.text() ? n.text().as_string() : Type;
            return true;
        }
        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Task");
            node.append_attribute("Type") = Type.c_str();
            return node;
        }
    };

    struct TaskInfoRequestDetails
    {
        string Id;
        string Type = "Output";
        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            auto t = n.child("Type");
            if (t) Type = t.text() ? t.text().as_string() : Type;
            return true;
        }
        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Task");
            node.append_attribute("Id") = Id.c_str();
            auto typeNode = node.append_child("Type");
            typeNode.text().set(Type.c_str());
            return node;
        }
    };

    struct TaskInfoRequest
    {
        string Id;
        int Source = 100;
        int Destination = 999;
        bool IncludeTaskDetails = false;
        optional<TaskInfoRequestDetails> Task;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Source = get_attr_int(n, "Source", Source);
            Destination = get_attr_int(n, "Destination", Destination);
            IncludeTaskDetails = get_attr_bool(n, "IncludeTaskDetails", IncludeTaskDetails);
            auto t = n.child("Task");
            if (t) { TaskInfoRequestDetails td; td.load(t); Task = std::move(td); }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("TaskInfoRequest");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Source") = Source;
            node.append_attribute("Destination") = Destination;
            node.append_attribute("IncludeTaskDetails") = IncludeTaskDetails;
            if (Task) Task->save(node);
            return node;
        }
    };

    // BoxDetails MUST be defined before TaskInfoResponseDetails (which uses optional<BoxDetails>)
    struct BoxDetails
    {
        int Number = 0;
        
        bool load(const pugi::xml_node& n)
        {
            Number = n.attribute("Number") ? n.attribute("Number").as_int() : n.text().as_int();
            return true;
        }
        
        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Box");
            node.append_attribute("Number") = Number;
            return node;
        }
    };

    struct TaskInfoResponseDetails
    {
        string Type;
        string Id;
        string Status;
        vector<Article> Articles;
        optional<BoxDetails> Box;

        bool load(const pugi::xml_node& n)
        {
            Type = get_attr(n, "Type");
            Id = get_attr(n, "Id");
            Status = get_attr(n, "Status");
            Articles.clear();
            for (auto a : n.children("Article")) { Article art; art.load(a); Articles.push_back(std::move(art)); }
            auto box = n.child("Box");
            if (box)
            {
                BoxDetails bd; bd.Number = box.attribute("Number") ? box.attribute("Number").as_int() : box.text().as_int();
                Box = std::move(bd);
            }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("Task");
            node.append_attribute("Type") = Type.c_str();
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Status") = Status.c_str();
            for (auto const& a : Articles) a.save(node);
            if (Box)
            {
                auto b = node.append_child("Box");
                b.append_attribute("Number") = Box->Number;
            }
            return node;
        }
    };

    struct TaskInfoResponse
    {
        string Id;
        int Source = 100;
        int Destination = 999;
        optional<TaskInfoResponseDetails> Task;

        bool load(const pugi::xml_node& n)
        {
            Id = get_attr(n, "Id");
            Source = get_attr_int(n, "Source", Source);
            Destination = get_attr_int(n, "Destination", Destination);
            auto t = n.child("Task");
            if (t) { TaskInfoResponseDetails trd; trd.load(t); Task = std::move(trd); }
            return true;
        }

        pugi::xml_node save(pugi::xml_node& parent) const
        {
            auto node = parent.append_child("TaskInfoResponse");
            node.append_attribute("Id") = Id.c_str();
            node.append_attribute("Source") = Source;
            node.append_attribute("Destination") = Destination;
            if (Task) Task->save(node);
            return node;
        }
    };

    // Wrapper for WWKS root element
    struct XmlWrapper
    {
        string Version = "2.0";
        string TimeStamp; // ISO string
        optional<HelloResponse> HelloResponseElement;
        optional<StatusResponse> StatusResponseElement;
        optional<StockInfoResponse> StockInfoResponseElement;
        optional<OutputResponse> OutputResponseElement;
        optional<OutputMessage> OutputMessageElement;
        optional<InputMessage> InputMessageElement;
        optional<TaskInfoResponse> TaskInfoResponseElement;

        bool load(const pugi::xml_node& root)
        {
            if (std::string(root.name()) != "WWKS") return false;
            Version = get_attr(root, "Version", Version.c_str());
            TimeStamp = get_attr(root, "TimeStamp");

            auto hr = root.child("HelloResponse");
            if (hr) { HelloResponse h; h.load(hr); HelloResponseElement = std::move(h); }

            auto sr = root.child("StatusResponse");
            if (sr) { StatusResponse s; s.load(sr); StatusResponseElement = std::move(s); }

            auto sir = root.child("StockInfoResponse");
            if (sir) { StockInfoResponse s; s.load(sir); StockInfoResponseElement = std::move(s); }

            auto orr = root.child("OutputResponse");
            if (orr) { OutputResponse o; o.load(root); OutputResponseElement = std::move(o); } // note: OutputResponse may be nested differently

            auto om = root.child("OutputMessage");
            if (om) { OutputMessage omm; omm.load(root); OutputMessageElement = std::move(omm); }

            auto im = root.child("InputMessage");
            if (im) { InputMessage imsg; imsg.load(im); InputMessageElement = std::move(imsg); }

            auto tir = root.child("TaskInfoResponse");
            if (tir) { TaskInfoResponse t; t.load(tir); TaskInfoResponseElement = std::move(t); }

            return true;
        }

        pugi::xml_document save() const
        {
            pugi::xml_document doc;
            auto root = doc.append_child("WWKS");
            root.append_attribute("Version") = Version.c_str();
            if (!TimeStamp.empty()) root.append_attribute("TimeStamp") = TimeStamp.c_str();
            if (HelloResponseElement) HelloResponseElement->save(root);
            if (StatusResponseElement) StatusResponseElement->save(root);
            if (StockInfoResponseElement) StockInfoResponseElement->save(root);
            if (OutputResponseElement) OutputResponseElement->save(root);
            if (OutputMessageElement) OutputMessageElement->save(root);
            if (InputMessageElement) InputMessageElement->save(root);
            if (TaskInfoResponseElement) TaskInfoResponseElement->save(root);
            return doc;
        }
    };

} // namespace RowaPickupSlim::XmlDefinitions

/*
What changed / reasoning:
- Implemented a single self-contained C++ translation of the C# XmlDefinitions types.
- Each type exposes minimal load/save helpers using pugixml so you can parse incoming XML into these structs and create XML from them.
- Dates are kept as ISO strings for simplicity; adapt to std::chrono parsing if you need strict date types.
- This file assumes pugixml's header "pugixml.hpp" is available in your include paths.

Next steps (suggested):
- Split these definitions into header (.h) and implementation (.cpp) if you prefer conventional organization.
- Add more robust error handling / logging on load failures.
- Add unit tests with representative sample XML documents copied from your C# expectations.
*/
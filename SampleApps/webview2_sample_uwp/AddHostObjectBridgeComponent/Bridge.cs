using System.Collections.Generic;
using Windows.Foundation;

namespace AddHostObjectBridgeComponent
{
    public sealed class Bridge
    {
        private List<string> _items = new List<string>();
        public event TypedEventHandler<Bridge, IList<string>> ItemsChangedEvent;

        public Bridge() { }

        public string StringProperty { get; set; } = "example";

        public void ClearItems()
        {
            _items.Clear();
            PostItemsChangedEvent();
        }

        public void AppendToItems(string item)
        {
            _items.Add(item);
            PostItemsChangedEvent();
        }

        private void PostItemsChangedEvent()
        {
            if (ItemsChangedEvent != null)
            {
                ItemsChangedEvent(this, _items);
            }
        }
    }
}


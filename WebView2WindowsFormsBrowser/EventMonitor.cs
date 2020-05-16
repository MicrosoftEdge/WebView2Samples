// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace WebView2WindowsFormsBrowser
{
    // A window that shows events that dispatch from a WebView2
    // and its correpsonding CoreWebView2.
    public partial class EventMonitor : Form
    {
        // The WebView2 & CoreWebView2 of which we will monitor events
        private CoreWebView2 _coreWebView2;
        private WebView2 _webview2;

        // Create a new event monitor window that monitors events from
        // the provided WebView2 and its corresponding CoreWebView2.
        public EventMonitor(WebView2 webview2)
        {
            _webview2 = webview2;
            _coreWebView2 = webview2.CoreWebView2;

            InitializeComponent();
            LayoutControls();

            SubscribeToEvents();
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this._clearButton = new System.Windows.Forms.Button();
            this._eventsListBox = new System.Windows.Forms.ListBox();
            this._eventDetailsListBox = new System.Windows.Forms.ListBox();
            this.SuspendLayout();
            // 
            // clearButton
            // 
            this._clearButton.Location = new System.Drawing.Point(0, 0);
            this._clearButton.Name = "clearButton";
            this._clearButton.Size = new System.Drawing.Size(75, 23);
            this._clearButton.TabIndex = 1;
            this._clearButton.Text = "Clear";
            this._clearButton.UseVisualStyleBackColor = true;
            this._clearButton.Click += ClearButton_Click;
            //
            // eventsListBox
            //
            this._eventsListBox.Location = new System.Drawing.Point(0, this._clearButton.Location.Y + this._clearButton.Size.Height);
            this._eventsListBox.Name = "eventsListBox";
            this._eventsListBox.Size = new System.Drawing.Size(this.Width / 2, this.Height - this._clearButton.Size.Height);
            this._eventsListBox.TabIndex = 2;
            this._eventsListBox.Text = "Events";
            this._eventsListBox.SelectedIndexChanged += EventsListBox_SelectedIndexChanged;
            //
            // eventDetailsListBox
            //
            this._eventDetailsListBox.Location = new System.Drawing.Point(this._eventsListBox.Location.X + this._eventsListBox.Size.Width, this._eventsListBox.Location.Y);
            this._eventDetailsListBox.Name = "eventDetailsListBox";
            this._eventDetailsListBox.Size = new System.Drawing.Size(this._eventsListBox.Size.Width, this._eventsListBox.Size.Height);
            this._eventDetailsListBox.TabIndex = 2;
            this._eventDetailsListBox.Text = "Event details";

            this.Controls.Add(this._clearButton);
            this.Controls.Add(this._eventsListBox);
            this.Controls.Add(this._eventDetailsListBox);

            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Text = "EventMonitor";

            this.ResumeLayout(false);
            this.PerformLayout();
        }

        protected override void OnResize(EventArgs e)
        {
            LayoutControls();
        }

        private void LayoutControls()
        {
            this._clearButton.Location = new System.Drawing.Point(0, 0);
            this._clearButton.Size = new System.Drawing.Size(75, 23);

            this._eventsListBox.Location = new System.Drawing.Point(0, this._clearButton.Location.Y + this._clearButton.Size.Height);
            this._eventsListBox.Size = new System.Drawing.Size(this.ClientSize.Width / 4, this.ClientSize.Height - this._clearButton.Size.Height);

            this._eventDetailsListBox.Location = new System.Drawing.Point(this._eventsListBox.Location.X + this._eventsListBox.Size.Width, this._eventsListBox.Location.Y);
            this._eventDetailsListBox.Size = new System.Drawing.Size(this.ClientSize.Width * 3 / 4, this._eventsListBox.Size.Height);
        }

        private void ClearButton_Click(object sender, System.EventArgs e)
        {
            _eventsListData.Clear();
            SetListBoxFromListData();
        }

        private void EventsListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            SetListBoxFromListData();
        }

        // Update the controls _eventsListBox and _eventDetailsListBox to match
        // what is stored in _eventsListData.
        // If the perf is poor here we could replace this general purpose function
        // to instead take hints about what just changed and only act on that.
        // For example, to know that we just added a single entry to the end and then
        // only add that to the _eventsListBox.Items end.
        private void SetListBoxFromListData()
        {
            if (!_internalStateChange)
            {
                _internalStateChange = true;

                // First adjust the size of the Items to match our data.
                int countDifference = _eventsListData.Count - _eventsListBox.Items.Count;
                if (countDifference > 0)
                {
                    // There's more entries in the data, so increase the size of the listboxes items.
                    _eventsListBox.Items.AddRange(Enumerable.Repeat("", countDifference).ToArray<string>());
                }
                else if (countDifference < 0)
                {
                    // If there's more entries in the listbox we need to remove entries.
                    countDifference = -countDifference;
                    while (countDifference > 0)
                    {
                        _eventsListBox.Items.RemoveAt(_eventsListBox.Items.Count - 1);
                        --countDifference;
                    }
                }

                // Then change the name of each item in the list box to match
                // what's in our data.
                for (int index = 0; index < _eventsListData.Count; ++index)
                {
                    var eventListDataItem = _eventsListData[index];
                    if (index < _eventsListBox.Items.Count)
                    {
                        if ((string)_eventsListBox.Items[index] != eventListDataItem.Key)
                        {
                            _eventsListBox.Items[index] = eventListDataItem.Key;
                        }
                    }
                }

                _eventDetailsListBox.Items.Clear();
                if (_eventsListBox.SelectedIndex >= 0)
                {
                    var eventListDataItem = _eventsListData[_eventsListBox.SelectedIndex];
                    foreach (var detailsDataItem in eventListDataItem.Value)
                    {
                        _eventDetailsListBox.Items.Add(detailsDataItem.Key + " = " + detailsDataItem.Value);
                    }
                }
                _internalStateChange = false;
            }
        }

        // Class that we use to associate the extra info of the name of the
        // event with our event handler on the EventMonitor.
        internal class EventHandlerContext
        {
            private string _eventName;
            private EventMonitor _eventMonitor;
            public EventHandlerContext(EventMonitor eventMonitor, string eventName)
            {
                _eventName = eventName;
                _eventMonitor = eventMonitor;
            }

            public void HandleEvent(object sender, object eventArgs)
            {
                _eventMonitor.HandleEvent(_eventName, sender, eventArgs);
            }
        }

        private void SubscribeToAllEventsOnObject(string name, object target)
        {
            foreach (EventInfo eventInfo in target.GetType().GetEvents())
            {
                EventHandlerContext context = new EventHandlerContext(this, name + eventInfo.Name);
                MethodInfo methodInfo = context.GetType().GetMethod("HandleEvent");
                Delegate handler = Delegate.CreateDelegate(eventInfo.EventHandlerType, context, methodInfo);
                eventInfo.AddEventHandler(target, handler);
            }
        }

        private void SubscribeToEvents()
        {
            SubscribeToAllEventsOnObject("CoreWebView2.", _coreWebView2);
            SubscribeToAllEventsOnObject("WebView2.", _webview2);
        }

        private static void AddObjectPropertiesToDetails(string objectName, object target, List<KeyValuePair<string, string>> details)
        {
            if (target != null)
            {
                try
                {
                    foreach (var propertyInfo in target.GetType().GetProperties())
                    {
                        details.Add(new KeyValuePair<string, string>(
                            objectName + propertyInfo.Name,
                            "" + propertyInfo.GetValue(target)));
                    }
                }
                catch (System.Reflection.TargetInvocationException)
                {
                    // If the CoreWebView2 is closed then it will throw an exception on any access.
                    // In that case, just skip CoreWebView2 and don't show any of its properties.
                }
            }
        }

        protected void HandleEvent(string eventName, object sender, object eventArgs)
        {
            List<KeyValuePair<string, string>> details = new List<KeyValuePair<string, string>>();

            AddObjectPropertiesToDetails(eventArgs.GetType().Name + ".", eventArgs, details);
            AddObjectPropertiesToDetails("CoreWebView2.", _coreWebView2, details);
            AddObjectPropertiesToDetails("CoreWebView2.Settings.", _coreWebView2.Settings, details);
            AddObjectPropertiesToDetails("WebView2.", _webview2, details);

            _eventsListData.Add(new KeyValuePair<string, List<KeyValuePair<string, string>>>(eventName, details));

            SetListBoxFromListData();
        }

        // Button to clear all the data from the _eventsListData and corresponding _eventsListBox
        private System.Windows.Forms.Button _clearButton;

        // The ListBox listing all the events.
        private System.Windows.Forms.ListBox _eventsListBox;

        // The ListBox listing all the properties from an event selected in _eventsListBox.
        private System.Windows.Forms.ListBox _eventDetailsListBox;

        // The underlying data of what is displayed in _eventsListBox and _eventDetailsListBox.
        // It is a list of pairs that represent an event. Each event entry is a pair where the key is the
        // name of the event that fired and the value is a list of pairs that represent the
        // properties of the event. Each property is a pair where the key is the name of the
        // property, and the value is the value of the property.
        // The list of events is sorted by the time at which they were dispatched from earliest to latest.
        // The list of properties is not in any particular order except grouped by object for easier
        // reading.
        private List<KeyValuePair<string, List<KeyValuePair<string, string>>>> _eventsListData =
            new List<KeyValuePair<string, List<KeyValuePair<string, string>>>>();

        // Set to true when we update the UI from the data and don't want to accidentally interpret
        // those UI changes as things we need to then apply back to the data and get stuck in a loop.
        private bool _internalStateChange = false;
    }
}

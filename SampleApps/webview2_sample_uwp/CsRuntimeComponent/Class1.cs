using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CsRuntimeComponent
{
    public sealed class CsRuntimeClass
    {
        public CsRuntimeClass() { }
        public string GetStringPropertyValue() { return this.StringProperty; }
        public string StringProperty { get; set; } = "example";

        public void InvokeExampleEvent()
        {
            this.ExampleEvent.Invoke(this, this.StringProperty);
        }

        public event Windows.Foundation.TypedEventHandler<CsRuntimeClass, string> ExampleEvent;
    }
}

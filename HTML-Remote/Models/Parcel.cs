using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace WebUI.Models
{
    public class Parcel
    {
        public string client { get; set; }
        public int tran { get; set; } = 0;
        public string[] fields { get; set; }
        public string[] values { get; set; }

    }
}
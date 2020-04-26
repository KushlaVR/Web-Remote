using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace WebUI.Controllers
{
    public class ApiController : Controller
    {
        // GET: Api
        public ActionResult PipeName()
        {
            return new ContentResult() {ContentType = "text/plain", Content = "ws://" + Request.Url.Host + ":" + Request.Url.Port +"/pipe.ashx" };
        }
    }
}
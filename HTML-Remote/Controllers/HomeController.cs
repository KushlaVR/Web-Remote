using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace WebUI.Controllers
{
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            return View();
        }

        public ActionResult Chat()
        {
            ViewBag.Message = "Chat";
            return View();
        }
         public ActionResult Stream()
        {
            ViewBag.Message = "Stream";
            return View();
        }
    }
}
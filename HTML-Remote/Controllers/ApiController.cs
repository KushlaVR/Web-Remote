using Newtonsoft.Json;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Security.Policy;
using System.Threading;
using System.Web;
using System.Web.Mvc;
using WebUI.Models;

namespace WebUI.Controllers
{
    public class ApiController : Controller
    {

        //private static readonly ConcurrentQueue<StreamWriter> _streammessage = new ConcurrentQueue<StreamWriter>();
        private static readonly WorkSpace ws = new WorkSpace();

        //GET: PipeName
        public ActionResult PipeName()
        {
            return new ContentResult() { ContentType = "text/plain", Content = "ws://" + Request.Url.Host + ":" + Request.Url.Port + "/pipe.ashx" };
        }


        public ActionResult EventSourceName()
        {
            return new ContentResult() { ContentType = "text/plain", Content = "http://" + Request.Url.Host + ":" + Request.Url.Port + "/api/get?" + Server.UrlEncode("{\"client\":\"" + (Guid.NewGuid().ToString()) + "\"}") };
        }

        /// <summary>
        /// When the user makes a GET request, 
        /// we’ll create a new HttpResponseMessage using PushStreamContent object 
        /// and text/event-stream content type. 
        /// PushStreamContent takes an Action&lt;Stream, HttpContentHeaders, TransportContext&gt; onStreamAvailable parameter in the constructor, and that in turn allows us to manipulate the response stream.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        public HttpResponseMessage Get()
        {
            try
            {
                var json = Server.UrlDecode(Request.QueryString[null]);
                Parcel m = JsonConvert.DeserializeObject<Parcel>(json);
                StreamClient client = ws.ClientByID(m.client) as StreamClient;
                if (client == null)
                {
                    client = new StreamClient() { Response = Response, clientID = m.client };
                    //client.SessionID = HttpContext.Session.SessionID;
                    ws.AddClient(client);
                    Response.ContentType = "text/event-stream";
                    Response.Headers.Add("Connection", "Keep-Alive");
                }
                else
                {
                    client.Response = Response;
                }

                do
                {
                    if (!ws.SendOne(client).Result)
                    {
                        return null;
                    };
                    Response.Flush();
                    Thread.Sleep(200);
                } while (true);
            }
            catch (Exception ex)
            {

            }
            return null;
        }

        /// <summary>
        /// When the user makes a POST request, using model binidng we pull a 
        /// Message object out of the request and pass it off to MessageCallback
        /// </summary>
        public void Post()
        {
            //var json = Server.UrlDecode(Request.QueryString[null]);
            var rdr = new StreamReader(Request.GetBufferedInputStream());
            var json = rdr.ReadToEnd();
            Parcel m = JsonConvert.DeserializeObject<Parcel>(json);
            Client client = ws.ClientByID(m.client);
            if (client != null)
            {
                client.processParcel(m);
            }
        }
    }

}

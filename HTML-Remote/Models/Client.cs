using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web;

namespace WebUI.Models
{
    public class Client
    {
        public static int tran = 0;

        public WorkSpace ws;
        public List<String> format;
        public Dictionary<string, string> received;
        public Dictionary<string, string> sent = new Dictionary<string, string>();
        //public string SessionID { get; set; }
        public string clientID { get; internal set; }

        public bool processParcel(Parcel v)
        {
            if (v.tran > tran) tran = v.tran;

            if (v.fields != null)
            {
                format = new List<string>(v.fields);
                return true;
            }

            if (format != null && v.values != null)
            {
                Dictionary<string, string> values = new Dictionary<string, string>();
                for (int i = 0; i < format.Count; i++)
                {
                    if (i < v.values.Length)
                    {
                        values.Add(format[i], v.values[i]);
                    }
                    else
                    {
                        values.Add(format[i], "");
                    }
                }
                received = values;
            }

            if (received != null)
            {
                ws.updateValues(received);
            }

            return true;
        }

        public async Task<bool> sendAsync(Dictionary<string, string> values)
        {
            if (format == null) return true;
            if (values == null) return true;
            if (!changed(values)) return true;
            var v = new List<String>();
            for (int i = 0; i < format.Count; i++)
            {
                string key = format[i];
                if (values.ContainsKey(key))
                    v.Add(values[key]);
                else
                    v.Add("");
            }

            try
            {
                string ret = null;
                ret = JsonConvert.SerializeObject(new Parcel() {tran = tran, values = v.ToArray() });
                if (await WeriteJson(ret))
                {
                    foreach (string key in values.Keys)
                    {
                        if (!sent.ContainsKey(key))
                            sent.Add(key, values[key]);
                        else
                            sent[key] = values[key];
                    }
                }
                return true;
            }
            catch (ObjectDisposedException)
            {
            }
            return false;
        }

        private bool changed(Dictionary<string, string> values)
        {
            if (sent == null) return true;
            foreach (string key in values.Keys)
            {
                if (!sent.ContainsKey(key))
                {
                    return true;
                }
                else
                {
                    if (values[key] != sent[key])
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        internal virtual async Task<string> readJson()
        {
            return await Task.FromResult("{}");
        }

        internal virtual async Task<bool> WeriteJson(string json)
        {
            return await Task<bool>.FromResult(true);
        }
    
    }

    public class WebSocketClient : Client
    {
        public WebSocket socket;

        internal override async Task<string> readJson()
        {
            var buffer = new ArraySegment<byte>(new byte[1024]);
            var result = await socket.ReceiveAsync(buffer, CancellationToken.None);
            Stream str = new MemoryStream(buffer.ToArray());
            StreamReader reader = new StreamReader(str);
            return reader.ReadToEnd();
        }

        internal virtual async Task<bool> receiveNextAsync()
        {
            return processParcel(JsonConvert.DeserializeObject<Parcel>(await readJson()));
        }

        internal override async Task<bool> WeriteJson(string json)
        {
            if (socket.State == WebSocketState.Open)
            {
                ArraySegment<byte> data = new ArraySegment<byte>(Encoding.ASCII.GetBytes(json));
                await socket.SendAsync(data, WebSocketMessageType.Text, true, CancellationToken.None);
                return await base.WeriteJson(json);
            }
            return false;
        }
    
    }

    public class StreamClient : Client
    {
        public HttpResponseBase Response { get; internal set; }

        internal override async Task<bool> WeriteJson(string json)
        {
            Response.Write("data:" + json + "\n\n");
            Response.Flush();
            return await Task.FromResult(true);
        }

    }
}
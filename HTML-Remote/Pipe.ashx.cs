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
using System.Web.WebSockets;

namespace WebUI
{
    /// <summary>
    /// Summary description for Pipe
    /// </summary>
    public class Pipe : IHttpHandler
    {

        public class Parcel
        {
            public string[] fields { get; set; }
            public string[] values { get; set; }

        }

        public class Client
        {

            public WebSocket socket;

            public List<String> format;
            public Dictionary<string, string> received;

            public async Task<bool> sendAsync(Dictionary<string, string> values)
            {
                if (format == null) return true;
                var v = new List<String>();
                try
                {
                    Locker.EnterWriteLock();
                    for (int i = 0; i < format.Count; i++)
                    {
                        string key = format[i];
                        if (values.ContainsKey(key))
                            v.Add(values[key]);
                        else
                            v.Add("");
                    }
                }
                finally
                {
                    Locker.ExitWriteLock();
                }

                try
                {
                    if (socket.State == WebSocketState.Open)
                    {
                        string ret = null;
                        ret = JsonConvert.SerializeObject(new Parcel() { values = v.ToArray() });
                        ArraySegment<byte> data = new ArraySegment<byte>(Encoding.ASCII.GetBytes(ret));
                        await socket.SendAsync(data, WebSocketMessageType.Text, true, CancellationToken.None);
                        return true;
                    }
                }
                catch (ObjectDisposedException)
                {
                }
                return false;
            }

            internal async Task<bool> receiveNextAsync()
            {
                var buffer = new ArraySegment<byte>(new byte[1024]);

                var result = await socket.ReceiveAsync(buffer, CancellationToken.None);
                Stream str = new MemoryStream(buffer.ToArray());
                StreamReader reader = new StreamReader(str);
                string json = reader.ReadToEnd();

                var v = JsonConvert.DeserializeObject<Parcel>(json);
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
                return true;
            }
        }

        private static readonly Dictionary<string, string> currentValues = new Dictionary<string, string>();
        private static readonly Dictionary<string, string> sendValues = new Dictionary<string, string>();


        private static Timer timer = new Timer(timer_tick, null, 0, 500);

        private static void timer_tick(object state)
        {
            if (timer_tickAsync(state).Result)
            {

            }
        }

        private static async Task<bool> timer_tickAsync(object state)
        {
            //if (currentValues.ContainsKey("gun_x"))
            //{
            //    int v = int.Parse(currentValues["gun_x"]) + 1;
            //    if (v > 100) v = -100;
            //    currentValues["gun_x"] = v.ToString();
            //}
            return await sendAll();
        }

        private static Task<bool> sendAll()
        {

            bool changed = false;
            Locker.EnterWriteLock();
            try
            {
                foreach (string key in currentValues.Keys)
                {
                    if (!sendValues.ContainsKey(key))
                    {
                        changed = true;
                        break;
                    }
                    else
                    {
                        if (currentValues[key] != sendValues[key])
                        {
                            changed = true;
                            break;
                        }
                    }
                }
            }
            finally
            {
                Locker.ExitWriteLock();
            }

            

            if (changed)
            {
                //Передаём сообщение всем клиентам
                for (int i = 0; i < Clients.Count; i++)
                {
                    Client nextClient = Clients[i];
                    if (!nextClient.sendAsync(currentValues).Result)
                    {
                        Locker.EnterWriteLock();
                        try
                        {
                            Clients.Remove(nextClient);
                            i--;
                        }
                        finally
                        {
                            Locker.ExitWriteLock();
                        }
                    };
                }
                foreach (string key in currentValues.Keys)
                {
                    if (!sendValues.ContainsKey(key))
                        sendValues.Add(key, currentValues[key]);
                    else
                        sendValues[key] = currentValues[key];
                }
            }

            return Task.FromResult(true);

        }

        public void ProcessRequest(HttpContext context)
        {
            if (context.IsWebSocketRequest)
                context.AcceptWebSocketRequest(WebSocketRequest);
        }

        public bool IsReusable
        {
            get
            {
                return false;
            }
        }

        // Список всех клиентов
        private static readonly List<Client> Clients = new List<Client>();

        // Блокировка для обеспечения потокабезопасности
        private static readonly ReaderWriterLockSlim Locker = new ReaderWriterLockSlim();

        private async Task WebSocketRequest(AspNetWebSocketContext context)
        {
            // Получаем сокет клиента из контекста запроса
            var client = new Client() { socket = context.WebSocket };
            // Добавляем его в список клиентов
            Locker.EnterWriteLock();
            try
            {
                Clients.Add(client);
            }
            finally
            {
                Locker.ExitWriteLock();
            }

            // Слушаем его
            while (true)
            {
                if (await client.receiveNextAsync())
                {
                    if (client.received != null)
                    {

                        Locker.EnterWriteLock();
                        try
                        {
                            foreach (string key in client.received.Keys)
                            {
                                if (currentValues.ContainsKey(key))
                                {
                                    currentValues[key] = client.received[key];
                                }
                                else
                                {
                                    currentValues.Add(key, client.received[key]);
                                }
                            }

                        }
                        finally
                        {
                            Locker.ExitWriteLock();
                        }


                    }
                }
            }
        }

    }
}
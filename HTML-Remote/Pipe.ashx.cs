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

        public class Client
        {

            public WebSocket socket;

            public List<String> format;
            public Dictionary<string, string> received;

            public async Task<bool> sendAsync(Dictionary<string, string> values)
            {
                if (format == null) return true;

                ArraySegment<byte> data;
                try
                {
                    Locker.EnterWriteLock();
                    string ret = null;
                    ret = JsonConvert.SerializeObject(values);
                    data = new ArraySegment<byte>(Encoding.ASCII.GetBytes(ret));
                }
                finally
                {
                    Locker.ExitWriteLock();
                }

                try
                {
                    if (socket.State == WebSocketState.Open)
                    {
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

                var v = JsonConvert.DeserializeObject<Dictionary<string, string>>(json);
                if (v.ContainsKey("fields"))
                {
                    розпарсити формат
                    return true;
                }

                received = v;
                return true;
            }
        }

        private static readonly Dictionary<string, string> values = new Dictionary<string, string>();
        private static readonly Dictionary<string, string> sendValues = new Dictionary<string, string>();




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
                        bool changed = false;

                        Locker.EnterWriteLock();
                        try
                        {
                            foreach (string key in client.received.Keys)
                            {
                                if (values.ContainsKey(key))
                                {
                                    values[key] = client.received[key];
                                }
                                else
                                {
                                    values.Add(key, client.received[key]);
                                }
                            }

                            foreach (string key in values.Keys)
                            {
                                if (!sendValues.ContainsKey(key))
                                {
                                    changed = true;
                                    break;
                                }
                                else
                                {
                                    if (values[key] != sendValues[key])
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
                                if (!await client.sendAsync(values))
                                {
                                    Locker.EnterWriteLock();
                                    try
                                    {
                                        Clients.Remove(client);
                                        i--;
                                    }
                                    finally
                                    {
                                        Locker.ExitWriteLock();
                                    }
                                };
                            }
                        }
                    }



                }

            }
        }
    }
}
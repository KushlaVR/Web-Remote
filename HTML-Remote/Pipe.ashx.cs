using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web;
using System.Web.Script.Serialization;
using System.Web.WebSockets;

namespace WebUI
{
    /// <summary>
    /// Summary description for Pipe
    /// </summary>
    public class Pipe : IHttpHandler
    {

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
        private static readonly List<WebSocket> Clients = new List<WebSocket>();

        // Блокировка для обеспечения потокабезопасности
        private static readonly ReaderWriterLockSlim Locker = new ReaderWriterLockSlim();



        private async Task WebSocketRequest(AspNetWebSocketContext context)
        {
            // Получаем сокет клиента из контекста запроса
            var socket = context.WebSocket;

            // Добавляем его в список клиентов
            Locker.EnterWriteLock();
            try
            {
                Clients.Add(socket);
            }
            finally
            {
                Locker.ExitWriteLock();
            }

            // Слушаем его
            while (true)
            {
                var buffer = new ArraySegment<byte>(new byte[1024]);

                var result = await socket.ReceiveAsync(buffer, CancellationToken.None);
                Stream str = new MemoryStream(buffer.ToArray());
                StreamReader reader = new StreamReader(str);
                string json = reader.ReadToEnd();
                JavaScriptSerializer json_serializer = new JavaScriptSerializer();
                Dictionary<string, string> v = (Dictionary<string, string>)json_serializer.DeserializeObject(json);

                bool changed = false;


                Locker.EnterWriteLock();
                try
                {
                    foreach (string key in v.Keys)
                    {
                        if (values.ContainsKey(key))
                        {
                            values[key] = v[key];
                        }
                        else
                        {
                            values.Add(key, v[key]);
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
                    ArraySegment<byte> data;
                    try
                    {
                        Locker.EnterWriteLock();
                    string ret = null;
                        ret = json_serializer.Serialize(values);
                        data = new ArraySegment<byte>(Encoding.ASCII.GetBytes(ret));
                    }
                    finally
                    {
                        Locker.ExitWriteLock();
                    }

                    //Передаём сообщение всем клиентам
                    for (int i = 0; i < Clients.Count; i++)
                    {
                        WebSocket client = Clients[i];
                        try
                        {
                            if (client.State == WebSocketState.Open)
                            {
                                await client.SendAsync(data, WebSocketMessageType.Text, true, CancellationToken.None);
                            }
                        }
                        catch (ObjectDisposedException)
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
                        }
                    }
                }
            }
        }
    }
}
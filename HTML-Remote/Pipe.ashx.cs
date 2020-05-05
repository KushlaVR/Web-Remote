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
using WebUI.Models;

namespace WebUI
{
    /// <summary>
    /// Summary description for Pipe
    /// </summary>
    public class Pipe : IHttpHandler
    {
        private static readonly WorkSpace ws = new WorkSpace();

        private static  Timer timer = new Timer(timer_tick, null, 0, 500);

        private static  void timer_tick(object state)
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
            return await ws.sendAll();
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

        private async Task WebSocketRequest(AspNetWebSocketContext context)
        {
            // Получаем сокет клиента из контекста запроса
            var client = new WebSocketClient() { socket = context.WebSocket };
            // Добавляем его в список клиентов
            ws.AddClient(client);
            // Слушаем его
            while (true)
            {
                if (await client.receiveNextAsync())
                {
                   
                }
            }
        }

    }
}
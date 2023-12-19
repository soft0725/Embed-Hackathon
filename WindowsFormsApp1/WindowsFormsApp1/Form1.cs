using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Google.Protobuf.WellKnownTypes;
using MySql.Data.MySqlClient;
using Org.BouncyCastle.Tls;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace WindowsFormsApp1
{
    public partial class Form1 : Form
    {
        String Conn = "Server=localhost;Database=ProjectWinter;Uid=root;Pwd=1234";
        MqttClient client;
        string clientId;

        int IsStart = 0; //엔진 시작 
        int value;

        // chart 데이터 시각화 변수들 
        int temp_x = 0;
        int humi_x = 0;


        public Form1()
        {
            InitializeComponent();
            timer1.Interval = 100; //타이머 간격 100ms
            timer1.Start();  //타이머 시작    
        }

        // Form 로드 되면 실행 
        private void Form1_Load(object sender, EventArgs e)
        {
            string BrokerAddress = "broker.mqtt-dashboard.com";
            client = new MqttClient(BrokerAddress);

            client.MqttMsgPublishReceived += client_MqttMsgPublishReceived;

            clientId = Guid.NewGuid().ToString();
            client.Connect(clientId);

            String[] topic = { "nockanda/temp", "nockanda/humi", "nockanda/led" };
            byte[] qos = { 0, 0, 0 };
            client.Subscribe(topic, qos);
        }

        // 엔진 버튼 부분 
        private void button1_Click(object sender, EventArgs e)
        {
   
        }


        void client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            this.Invoke(new MethodInvoker(delegate ()
            {
                string ReceivedMessage = Encoding.UTF8.GetString(e.Message);

                if (e.Topic == "nockanda/temp" && IsStart == 1)
                {
                    double value = double.Parse(ReceivedMessage);
                    chart1.Series[0].Points.AddXY(temp_x, value);
                    using (MySqlConnection conn = new MySqlConnection(Conn))
                    {
                        conn.Open();
                        MySqlCommand msc = new MySqlCommand($"INSERT INTO temp values ({value}) ", conn);
                        msc.ExecuteNonQuery();
                    }
                    // 데이터셋의 갯수가 윈도우 사이즈를 초과했는가? 
                    if (chart1.Series[0].Points.Count > 10)
                    {
                        chart1.Series[0].Points.RemoveAt(0);
                    }
                    chart1.ChartAreas[0].AxisX.Maximum = temp_x;
                    chart1.ChartAreas[0].AxisX.Minimum = chart1.Series[0].Points[0].XValue;

                    double max = 0;
                    for(int i = 0;  i <  chart1.Series[0].Points.Count; i++)
                    {
                        if (max < chart1.Series[0].Points[i].YValues[0])
                        {
                            max = chart1.Series[0].Points[i].YValues[0];
                        }
                    }
                    chart1.ChartAreas[0].AxisY.Maximum = max+5;
                    temp_x++;
                }
                if (e.Topic == "nockanda/humi" && IsStart == 1)
                {
                    double value = double.Parse(ReceivedMessage);
                    using (MySqlConnection conn = new MySqlConnection(Conn))
                    {
                        conn.Open();
                        MySqlCommand msc = new MySqlCommand($"INSERT INTO humi values ({value}) ", conn);
                        msc.ExecuteNonQuery();
                    }
                    chart1.Series[1].Points.AddXY(humi_x, value);

                    // 데이터셋의 갯수가 윈도우 사이즈를 초과했는가? 
                    if (chart1.Series[1].Points.Count > 10)
                    {
                        chart1.Series[1].Points.RemoveAt(0);
                    }
                    chart1.ChartAreas[0].AxisX.Maximum = humi_x;
                    chart1.ChartAreas[0].AxisX.Minimum = chart1.Series[1].Points[0].XValue;

                    double max = 0;
                    for (int i = 0; i < chart1.Series[1].Points.Count; i++)
                    {
                        if (max < chart1.Series[1].Points[i].YValues[0])
                        {
                            max = chart1.Series[1].Points[i].YValues[0];
                        }
                    }
                    chart1.ChartAreas[0].AxisY.Maximum = max + 5;
                    humi_x++;
                }
               if(e.Topic == "nockanda/led")
               {
                    value = int.Parse(ReceivedMessage);
                    if (value == 1)
                    {
                        IsStart = 1;
                        button1.BackColor = Color.FromArgb(128, 255, 128);
                    }
                    else
                    {
                        IsStart = 0;
                        button1.BackColor = Color.FromArgb(255, 192, 192);
                    }
               }
            }));
        }

        // 시간 표시 
        private void timer1_Tick(object sender, EventArgs e)
        {
            label1.Text = DateTime.Now.ToString("F"); // label1에 현재날짜시간 표시, F:자세한 전체 날짜/시간
        }


        // 안씀 

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
        }

        private void groupBox3_Enter(object sender, EventArgs e)
        {

        }

        private void chart1_Click(object sender, EventArgs e)
        {

        }
    }
}

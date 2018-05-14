using System;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace BarGenerator
{
    public partial class Main : Form
    {
        [DllImport("BarCore.dll")]
        public static extern int generate(string outPath, int len, int distance, int maxCodes, int generateDesc, int detectDeadLoop);

        [DllImport("BarCore.dll")]
        public static extern int calcMinDistance(string inPath, int barLength, int idsColumn, ref int maxDist, ref double avgDist);

        private int m_codeLen, m_codesNum, m_hDist, m_realDist;
        private bool m_genDesc, m_success, m_successVerify, m_deadLoop;
        private BackgroundWorker bw = new BackgroundWorker();
        private int m_maxLen = 0;
        private double m_avgLen = 0;


        public Main()
        {
            InitializeComponent();
            bw.WorkerReportsProgress = true;
            bw.DoWork += new DoWorkEventHandler(bw_DoWork);
            bw.ProgressChanged += new ProgressChangedEventHandler(bw_ProgressChanged);
            bw.RunWorkerCompleted += new RunWorkerCompletedEventHandler(bw_RunWorkerCompleted);
        }

        private void btnSelectOutput_Click(object sender, EventArgs e)
        {
            if (sfdMain.ShowDialog() == DialogResult.OK)
                tbOutput.Text = sfdMain.FileName;
        }

        private double factorial(long n)
        {
            double res = 1;
            for (long i = 2; i <= n; i++)
                res *= i;
            return res;
        }

        private double cNK(long k, long n)
        {
            return factorial(n) / (factorial(k) * factorial(n - k));
        }

        private void btnProceed_Click(object sender, EventArgs e)
        {
            m_genDesc = cbGenDesc.Checked;

            m_codeLen = (int)nudCodeLength.Value;
            if (!Int32.TryParse(tbCodesNumber.Text, out m_codesNum))
            {
                MessageBox.Show("Please specify valid codes number", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            if (rb3.Checked)
                m_hDist = 3;
            else if (rb5.Checked)
                m_hDist = 5;
            else if (rb7.Checked)
                m_hDist = 7;
            else
                m_hDist = 9;

            if (tbOutput.Text == "")
            {
                MessageBox.Show("Please specify valid output file", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Check number of codes
/*            double sum = 0;
            for (int i = 1; i <= m_hDist; i++)
                sum += cNK(i, m_codeLen);
            long possible = (long)(Math.Pow(4, m_codeLen-1) / (1 + sum));

            MessageBox.Show(possible.ToString());*/


            this.Enabled = false;
            pbMain.Value = 0;
            pbMain.Visible = true;

            bw.RunWorkerAsync();
        }

        private void bw_DoWork(object sender, DoWorkEventArgs e)
        {
            int res;
            BackgroundWorker worker = sender as BackgroundWorker;

            m_success = false;
            m_deadLoop = false;
            res = generate(tbOutput.Text, m_codeLen, m_hDist, m_codesNum, cbGenDesc.Checked ? 1 : 0, cbDeadLoop.Checked ? 1 : 0);

            if (res != 0)
            {
                if (res == 9) // Dead Loop
                    m_deadLoop = true;
                return;
            }

            m_success = true;

            if (cbVerify.Checked)
            {
                m_successVerify = false;
                worker.ReportProgress(50);
                m_realDist = calcMinDistance(tbOutput.Text, m_codeLen, m_genDesc ? 1 : 0, ref m_maxLen, ref m_avgLen);

                if (m_realDist >= 0)
                    m_successVerify = true;
            }
            worker.ReportProgress(100);
        }

        private void bw_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            pbMain.Visible = false;
            this.Enabled = true;

            if (!m_success)
            {
                if (m_deadLoop)
                    MessageBox.Show("Dead loop detected during generation. Not all codes generated.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                else
                    MessageBox.Show("Error occured during generation", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            if (!cbVerify.Checked) // All ok
            {
                MessageBox.Show("Successfully generated", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            if (!m_successVerify)
            {
                MessageBox.Show("Error occured during verification", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            if (m_realDist < m_hDist) // Ups
            {
                MessageBox.Show(String.Format("Verification failed. Minimum Hamming distance is {0} while should be {1}", m_realDist, m_hDist), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            } 
            MessageBox.Show(String.Format("Verification succeed. Minimum Hamming distance is {0} with requested {1}. Maximum is {2}, average is {3}.", m_realDist, m_hDist, m_maxLen, m_avgLen), "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);

        }
        private void bw_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            pbMain.Value = e.ProgressPercentage;
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void checkBox2_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void btnCalc_Click(object sender, EventArgs e)
        {
            if (ofdMain.ShowDialog() != DialogResult.OK)
            {
                ofdMain.Dispose();
                return;
            }
            int maxLen = 0;
            double avgLen = 0;

            int minLen = calcMinDistance(ofdMain.FileName, 0, (int)nudColumn.Value, ref maxLen, ref avgLen);

            if (minLen < 0)
                MessageBox.Show("Error occured during calculation", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show(String.Format("Minimum hamming distance = {0}, maximum = {1}, average = {2}", minLen, maxLen, avgLen),
                    "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

    }
}


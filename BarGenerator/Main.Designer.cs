namespace BarGenerator
{
    partial class Main
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.tbCodesNumber = new System.Windows.Forms.TextBox();
            this.btnProceed = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.rb5 = new System.Windows.Forms.RadioButton();
            this.rb3 = new System.Windows.Forms.RadioButton();
            this.label4 = new System.Windows.Forms.Label();
            this.tbOutput = new System.Windows.Forms.TextBox();
            this.btnSelectOutput = new System.Windows.Forms.Button();
            this.cbGenDesc = new System.Windows.Forms.CheckBox();
            this.sfdMain = new System.Windows.Forms.SaveFileDialog();
            this.nudCodeLength = new System.Windows.Forms.NumericUpDown();
            this.rb7 = new System.Windows.Forms.RadioButton();
            this.rb9 = new System.Windows.Forms.RadioButton();
            this.cbVerify = new System.Windows.Forms.CheckBox();
            this.pbMain = new System.Windows.Forms.ProgressBar();
            this.cbFast = new System.Windows.Forms.CheckBox();
            this.cbDeadLoop = new System.Windows.Forms.CheckBox();
            this.btnCalc = new System.Windows.Forms.Button();
            this.ofdMain = new System.Windows.Forms.OpenFileDialog();
            this.nudColumn = new System.Windows.Forms.NumericUpDown();
            this.label5 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.nudCodeLength)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudColumn)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 33);
            this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(88, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Number of codes";
            // 
            // tbCodesNumber
            // 
            this.tbCodesNumber.Location = new System.Drawing.Point(106, 30);
            this.tbCodesNumber.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.tbCodesNumber.Name = "tbCodesNumber";
            this.tbCodesNumber.Size = new System.Drawing.Size(62, 20);
            this.tbCodesNumber.TabIndex = 1;
            this.tbCodesNumber.Text = "65536";
            // 
            // btnProceed
            // 
            this.btnProceed.Location = new System.Drawing.Point(232, 132);
            this.btnProceed.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.btnProceed.Name = "btnProceed";
            this.btnProceed.Size = new System.Drawing.Size(108, 23);
            this.btnProceed.TabIndex = 2;
            this.btnProceed.Text = "Proceed";
            this.btnProceed.UseVisualStyleBackColor = true;
            this.btnProceed.Click += new System.EventHandler(this.btnProceed_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 59);
            this.label2.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(64, 13);
            this.label2.TabIndex = 6;
            this.label2.Text = "Code length";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(11, 9);
            this.label3.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(94, 13);
            this.label3.TabIndex = 7;
            this.label3.Text = "Hamming distance";
            // 
            // rb5
            // 
            this.rb5.AutoSize = true;
            this.rb5.Location = new System.Drawing.Point(140, 7);
            this.rb5.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.rb5.Name = "rb5";
            this.rb5.Size = new System.Drawing.Size(31, 17);
            this.rb5.TabIndex = 8;
            this.rb5.Text = "5";
            this.rb5.UseVisualStyleBackColor = true;
            // 
            // rb3
            // 
            this.rb3.AutoSize = true;
            this.rb3.Checked = true;
            this.rb3.Location = new System.Drawing.Point(105, 7);
            this.rb3.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.rb3.Name = "rb3";
            this.rb3.Size = new System.Drawing.Size(31, 17);
            this.rb3.TabIndex = 9;
            this.rb3.TabStop = true;
            this.rb3.Text = "3";
            this.rb3.UseVisualStyleBackColor = true;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 108);
            this.label4.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(55, 13);
            this.label4.TabIndex = 10;
            this.label4.Text = "Output file";
            // 
            // tbOutput
            // 
            this.tbOutput.Location = new System.Drawing.Point(106, 106);
            this.tbOutput.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.tbOutput.Name = "tbOutput";
            this.tbOutput.ReadOnly = true;
            this.tbOutput.Size = new System.Drawing.Size(210, 20);
            this.tbOutput.TabIndex = 11;
            // 
            // btnSelectOutput
            // 
            this.btnSelectOutput.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnSelectOutput.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.btnSelectOutput.Location = new System.Drawing.Point(320, 106);
            this.btnSelectOutput.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.btnSelectOutput.Name = "btnSelectOutput";
            this.btnSelectOutput.Size = new System.Drawing.Size(20, 20);
            this.btnSelectOutput.TabIndex = 12;
            this.btnSelectOutput.Text = "...";
            this.btnSelectOutput.UseVisualStyleBackColor = true;
            this.btnSelectOutput.Click += new System.EventHandler(this.btnSelectOutput_Click);
            // 
            // cbGenDesc
            // 
            this.cbGenDesc.AutoSize = true;
            this.cbGenDesc.Checked = true;
            this.cbGenDesc.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbGenDesc.Location = new System.Drawing.Point(187, 32);
            this.cbGenDesc.Name = "cbGenDesc";
            this.cbGenDesc.Size = new System.Drawing.Size(153, 17);
            this.cbGenDesc.TabIndex = 14;
            this.cbGenDesc.Text = "Generate fake descriptions";
            this.cbGenDesc.UseVisualStyleBackColor = true;
            // 
            // sfdMain
            // 
            this.sfdMain.Filter = "CSV files|*.csv";
            // 
            // nudCodeLength
            // 
            this.nudCodeLength.Location = new System.Drawing.Point(106, 57);
            this.nudCodeLength.Minimum = new decimal(new int[] {
            4,
            0,
            0,
            0});
            this.nudCodeLength.Name = "nudCodeLength";
            this.nudCodeLength.Size = new System.Drawing.Size(62, 20);
            this.nudCodeLength.TabIndex = 15;
            this.nudCodeLength.Value = new decimal(new int[] {
            18,
            0,
            0,
            0});
            // 
            // rb7
            // 
            this.rb7.AutoSize = true;
            this.rb7.Location = new System.Drawing.Point(175, 7);
            this.rb7.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.rb7.Name = "rb7";
            this.rb7.Size = new System.Drawing.Size(31, 17);
            this.rb7.TabIndex = 16;
            this.rb7.Text = "7";
            this.rb7.UseVisualStyleBackColor = true;
            // 
            // rb9
            // 
            this.rb9.AutoSize = true;
            this.rb9.Location = new System.Drawing.Point(210, 7);
            this.rb9.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.rb9.Name = "rb9";
            this.rb9.Size = new System.Drawing.Size(31, 17);
            this.rb9.TabIndex = 17;
            this.rb9.Text = "9";
            this.rb9.UseVisualStyleBackColor = true;
            // 
            // cbVerify
            // 
            this.cbVerify.AutoSize = true;
            this.cbVerify.Location = new System.Drawing.Point(187, 58);
            this.cbVerify.Name = "cbVerify";
            this.cbVerify.Size = new System.Drawing.Size(135, 17);
            this.cbVerify.TabIndex = 19;
            this.cbVerify.Text = "Verify generated codes";
            this.cbVerify.UseVisualStyleBackColor = true;
            // 
            // pbMain
            // 
            this.pbMain.Location = new System.Drawing.Point(15, 132);
            this.pbMain.Name = "pbMain";
            this.pbMain.Size = new System.Drawing.Size(212, 23);
            this.pbMain.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.pbMain.TabIndex = 20;
            this.pbMain.Visible = false;
            // 
            // cbFast
            // 
            this.cbFast.AutoSize = true;
            this.cbFast.Location = new System.Drawing.Point(15, 83);
            this.cbFast.Name = "cbFast";
            this.cbFast.Size = new System.Drawing.Size(138, 17);
            this.cbFast.TabIndex = 21;
            this.cbFast.Text = "Less memory but slower";
            this.cbFast.UseVisualStyleBackColor = true;
            this.cbFast.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
            // 
            // cbDeadLoop
            // 
            this.cbDeadLoop.AutoSize = true;
            this.cbDeadLoop.Checked = true;
            this.cbDeadLoop.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbDeadLoop.Location = new System.Drawing.Point(187, 83);
            this.cbDeadLoop.Name = "cbDeadLoop";
            this.cbDeadLoop.Size = new System.Drawing.Size(108, 17);
            this.cbDeadLoop.TabIndex = 22;
            this.cbDeadLoop.Text = "Detect dead loop";
            this.cbDeadLoop.UseVisualStyleBackColor = true;
            this.cbDeadLoop.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
            // 
            // btnCalc
            // 
            this.btnCalc.Location = new System.Drawing.Point(231, 161);
            this.btnCalc.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.btnCalc.Name = "btnCalc";
            this.btnCalc.Size = new System.Drawing.Size(109, 23);
            this.btnCalc.TabIndex = 23;
            this.btnCalc.Text = "Calculate stats";
            this.btnCalc.UseVisualStyleBackColor = true;
            this.btnCalc.Click += new System.EventHandler(this.btnCalc_Click);
            // 
            // ofdMain
            // 
            this.ofdMain.Filter = "CSV files|*.csv";
            // 
            // nudColumn
            // 
            this.nudColumn.Location = new System.Drawing.Point(187, 164);
            this.nudColumn.Maximum = new decimal(new int[] {
            20,
            0,
            0,
            0});
            this.nudColumn.Name = "nudColumn";
            this.nudColumn.Size = new System.Drawing.Size(38, 20);
            this.nudColumn.TabIndex = 24;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(12, 166);
            this.label5.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(162, 13);
            this.label5.TabIndex = 25;
            this.label5.Text = "Number of column with Bar Code";
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(357, 185);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.nudColumn);
            this.Controls.Add(this.btnCalc);
            this.Controls.Add(this.cbDeadLoop);
            this.Controls.Add(this.cbFast);
            this.Controls.Add(this.pbMain);
            this.Controls.Add(this.cbVerify);
            this.Controls.Add(this.rb9);
            this.Controls.Add(this.rb7);
            this.Controls.Add(this.nudCodeLength);
            this.Controls.Add(this.cbGenDesc);
            this.Controls.Add(this.btnSelectOutput);
            this.Controls.Add(this.tbOutput);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.rb3);
            this.Controls.Add(this.rb5);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.btnProceed);
            this.Controls.Add(this.tbCodesNumber);
            this.Controls.Add(this.label1);
            this.Margin = new System.Windows.Forms.Padding(2, 3, 2, 3);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(373, 202);
            this.Name = "Main";
            this.Text = "Bar Generator";
            ((System.ComponentModel.ISupportInitialize)(this.nudCodeLength)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nudColumn)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbCodesNumber;
        private System.Windows.Forms.Button btnProceed;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.RadioButton rb5;
        private System.Windows.Forms.RadioButton rb3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox tbOutput;
        private System.Windows.Forms.Button btnSelectOutput;
        private System.Windows.Forms.CheckBox cbGenDesc;
        private System.Windows.Forms.SaveFileDialog sfdMain;
        private System.Windows.Forms.NumericUpDown nudCodeLength;
        private System.Windows.Forms.RadioButton rb7;
        private System.Windows.Forms.RadioButton rb9;
        private System.Windows.Forms.CheckBox cbVerify;
        private System.Windows.Forms.ProgressBar pbMain;
        private System.Windows.Forms.CheckBox cbFast;
        private System.Windows.Forms.CheckBox cbDeadLoop;
        private System.Windows.Forms.Button btnCalc;
        private System.Windows.Forms.OpenFileDialog ofdMain;
        private System.Windows.Forms.NumericUpDown nudColumn;
        private System.Windows.Forms.Label label5;
    }
}


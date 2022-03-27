namespace PIF_Viewer
{
    partial class ViewerForm
    {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
			this.components = new System.ComponentModel.Container();
			this.Status = new System.Windows.Forms.StatusStrip();
			this.STATUS_FS_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.FileSize_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.Spacer1 = new System.Windows.Forms.ToolStripStatusLabel();
			this.Status_IT_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.ImageType_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.Spacer2 = new System.Windows.Forms.ToolStripStatusLabel();
			this.Status_CT_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.ImageSize_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.Spacer3 = new System.Windows.Forms.ToolStripStatusLabel();
			this.Status_DataSizeLBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.DataSize_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.Spacer4 = new System.Windows.Forms.ToolStripStatusLabel();
			this.Status_ColorTable_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.ColorTable_LBL = new System.Windows.Forms.ToolStripStatusLabel();
			this.Spacer5 = new System.Windows.Forms.ToolStripStatusLabel();
			this.Menu = new System.Windows.Forms.MenuStrip();
			this.MainMenu_File = new System.Windows.Forms.ToolStripMenuItem();
			this.Menu_Open = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
			this.Menu_Export = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
			this.Menu_Close = new System.Windows.Forms.ToolStripMenuItem();
			this.MainMenu_Info = new System.Windows.Forms.ToolStripMenuItem();
			this.Menu_About = new System.Windows.Forms.ToolStripMenuItem();
			this.bindingSource1 = new System.Windows.Forms.BindingSource(this.components);
			this.ImageBox = new PIF_Viewer.SpecialPictureBox();
			this.Status.SuspendLayout();
			this.Menu.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.bindingSource1)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.ImageBox)).BeginInit();
			this.SuspendLayout();
			// 
			// Status
			// 
			this.Status.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.STATUS_FS_LBL,
            this.FileSize_LBL,
            this.Spacer1,
            this.Status_IT_LBL,
            this.ImageType_LBL,
            this.Spacer2,
            this.Status_CT_LBL,
            this.ImageSize_LBL,
            this.Spacer3,
            this.Status_DataSizeLBL,
            this.DataSize_LBL,
            this.Spacer4,
            this.Status_ColorTable_LBL,
            this.ColorTable_LBL,
            this.Spacer5});
			this.Status.Location = new System.Drawing.Point(0, 515);
			this.Status.Name = "Status";
			this.Status.Size = new System.Drawing.Size(752, 22);
			this.Status.TabIndex = 2;
			this.Status.Text = "Status";
			// 
			// STATUS_FS_LBL
			// 
			this.STATUS_FS_LBL.Name = "STATUS_FS_LBL";
			this.STATUS_FS_LBL.Size = new System.Drawing.Size(51, 17);
			this.STATUS_FS_LBL.Text = "File Size:";
			// 
			// FileSize_LBL
			// 
			this.FileSize_LBL.Name = "FileSize_LBL";
			this.FileSize_LBL.Size = new System.Drawing.Size(44, 17);
			this.FileSize_LBL.Text = "0 Bytes";
			// 
			// Spacer1
			// 
			this.Spacer1.Name = "Spacer1";
			this.Spacer1.Size = new System.Drawing.Size(33, 17);
			this.Spacer1.Spring = true;
			// 
			// Status_IT_LBL
			// 
			this.Status_IT_LBL.Name = "Status_IT_LBL";
			this.Status_IT_LBL.Size = new System.Drawing.Size(70, 17);
			this.Status_IT_LBL.Text = "Image Type:";
			// 
			// ImageType_LBL
			// 
			this.ImageType_LBL.Name = "ImageType_LBL";
			this.ImageType_LBL.Size = new System.Drawing.Size(47, 17);
			this.ImageType_LBL.Text = "RGB888";
			// 
			// Spacer2
			// 
			this.Spacer2.Name = "Spacer2";
			this.Spacer2.Size = new System.Drawing.Size(33, 17);
			this.Spacer2.Spring = true;
			// 
			// Status_CT_LBL
			// 
			this.Status_CT_LBL.Name = "Status_CT_LBL";
			this.Status_CT_LBL.Size = new System.Drawing.Size(66, 17);
			this.Status_CT_LBL.Text = "Image Size:";
			// 
			// ImageSize_LBL
			// 
			this.ImageSize_LBL.Name = "ImageSize_LBL";
			this.ImageSize_LBL.Size = new System.Drawing.Size(25, 17);
			this.ImageSize_LBL.Text = "0, 0";
			// 
			// Spacer3
			// 
			this.Spacer3.Name = "Spacer3";
			this.Spacer3.Size = new System.Drawing.Size(33, 17);
			this.Spacer3.Spring = true;
			// 
			// Status_DataSizeLBL
			// 
			this.Status_DataSizeLBL.Name = "Status_DataSizeLBL";
			this.Status_DataSizeLBL.Size = new System.Drawing.Size(57, 17);
			this.Status_DataSizeLBL.Text = "Data Size:";
			// 
			// DataSize_LBL
			// 
			this.DataSize_LBL.Name = "DataSize_LBL";
			this.DataSize_LBL.Size = new System.Drawing.Size(44, 17);
			this.DataSize_LBL.Text = "0 Bytes";
			// 
			// Spacer4
			// 
			this.Spacer4.Name = "Spacer4";
			this.Spacer4.Size = new System.Drawing.Size(33, 17);
			this.Spacer4.Spring = true;
			// 
			// Status_ColorTable_LBL
			// 
			this.Status_ColorTable_LBL.Name = "Status_ColorTable_LBL";
			this.Status_ColorTable_LBL.Size = new System.Drawing.Size(69, 17);
			this.Status_ColorTable_LBL.Text = "Color Table:";
			// 
			// ColorTable_LBL
			// 
			this.ColorTable_LBL.Name = "ColorTable_LBL";
			this.ColorTable_LBL.Size = new System.Drawing.Size(98, 17);
			this.ColorTable_LBL.Text = "0 Colors / 0 Bytes";
			// 
			// Spacer5
			// 
			this.Spacer5.Name = "Spacer5";
			this.Spacer5.Size = new System.Drawing.Size(33, 17);
			this.Spacer5.Spring = true;
			// 
			// Menu
			// 
			this.Menu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenu_File,
            this.MainMenu_Info});
			this.Menu.Location = new System.Drawing.Point(0, 0);
			this.Menu.Name = "Menu";
			this.Menu.Size = new System.Drawing.Size(752, 24);
			this.Menu.TabIndex = 3;
			this.Menu.Text = "menuStrip1";
			// 
			// MainMenu_File
			// 
			this.MainMenu_File.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.Menu_Open,
            this.toolStripSeparator1,
            this.Menu_Export,
            this.toolStripSeparator2,
            this.Menu_Close});
			this.MainMenu_File.Name = "MainMenu_File";
			this.MainMenu_File.Size = new System.Drawing.Size(37, 20);
			this.MainMenu_File.Text = "&File";
			// 
			// Menu_Open
			// 
			this.Menu_Open.Name = "Menu_Open";
			this.Menu_Open.Size = new System.Drawing.Size(158, 22);
			this.Menu_Open.Text = "&Open PIF Image";
			this.Menu_Open.Click += new System.EventHandler(this.Menu_Open_Click);
			// 
			// toolStripSeparator1
			// 
			this.toolStripSeparator1.Name = "toolStripSeparator1";
			this.toolStripSeparator1.Size = new System.Drawing.Size(155, 6);
			// 
			// Menu_Export
			// 
			this.Menu_Export.Name = "Menu_Export";
			this.Menu_Export.Size = new System.Drawing.Size(158, 22);
			this.Menu_Export.Text = "&Export to...";
			this.Menu_Export.Click += new System.EventHandler(this.Menu_Export_Click);
			// 
			// toolStripSeparator2
			// 
			this.toolStripSeparator2.Name = "toolStripSeparator2";
			this.toolStripSeparator2.Size = new System.Drawing.Size(155, 6);
			// 
			// Menu_Close
			// 
			this.Menu_Close.Name = "Menu_Close";
			this.Menu_Close.Size = new System.Drawing.Size(158, 22);
			this.Menu_Close.Text = "&Close";
			this.Menu_Close.Click += new System.EventHandler(this.Menu_Close_Click);
			// 
			// MainMenu_Info
			// 
			this.MainMenu_Info.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.Menu_About});
			this.MainMenu_Info.Name = "MainMenu_Info";
			this.MainMenu_Info.Size = new System.Drawing.Size(40, 20);
			this.MainMenu_Info.Text = "&Info";
			// 
			// Menu_About
			// 
			this.Menu_About.Name = "Menu_About";
			this.Menu_About.Size = new System.Drawing.Size(107, 22);
			this.Menu_About.Text = "&About";
			// 
			// ImageBox
			// 
			this.ImageBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.ImageBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.ImageBox.Location = new System.Drawing.Point(0, 25);
			this.ImageBox.Margin = new System.Windows.Forms.Padding(1);
			this.ImageBox.Name = "ImageBox";
			this.ImageBox.Size = new System.Drawing.Size(752, 489);
			this.ImageBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
			this.ImageBox.TabIndex = 1;
			this.ImageBox.TabStop = false;
			// 
			// ViewerForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(752, 537);
			this.Controls.Add(this.Status);
			this.Controls.Add(this.ImageBox);
			this.Controls.Add(this.Menu);
			this.MainMenuStrip = this.Menu;
			this.MinimumSize = new System.Drawing.Size(768, 576);
			this.Name = "ViewerForm";
			this.Text = "PIF Image Viewer";
			this.Status.ResumeLayout(false);
			this.Status.PerformLayout();
			this.Menu.ResumeLayout(false);
			this.Menu.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.bindingSource1)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.ImageBox)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.StatusStrip Status;
        private System.Windows.Forms.MenuStrip Menu;
        private System.Windows.Forms.ToolStripMenuItem MainMenu_File;
        private System.Windows.Forms.ToolStripMenuItem Menu_Open;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem Menu_Export;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem Menu_Close;
        private System.Windows.Forms.ToolStripMenuItem MainMenu_Info;
        private System.Windows.Forms.ToolStripMenuItem Menu_About;
        private System.Windows.Forms.ToolStripStatusLabel STATUS_FS_LBL;
        private System.Windows.Forms.ToolStripStatusLabel FileSize_LBL;
        private System.Windows.Forms.ToolStripStatusLabel Spacer1;
        private System.Windows.Forms.ToolStripStatusLabel Status_IT_LBL;
        private System.Windows.Forms.ToolStripStatusLabel ImageType_LBL;
        private System.Windows.Forms.ToolStripStatusLabel Spacer2;
        private System.Windows.Forms.ToolStripStatusLabel Status_CT_LBL;
        private System.Windows.Forms.ToolStripStatusLabel ImageSize_LBL;
        private System.Windows.Forms.ToolStripStatusLabel Spacer3;
        private System.Windows.Forms.ToolStripStatusLabel Status_DataSizeLBL;
        private System.Windows.Forms.ToolStripStatusLabel DataSize_LBL;
        private System.Windows.Forms.ToolStripStatusLabel Spacer4;
        private System.Windows.Forms.ToolStripStatusLabel Status_ColorTable_LBL;
        private System.Windows.Forms.ToolStripStatusLabel ColorTable_LBL;
        private System.Windows.Forms.ToolStripStatusLabel Spacer5;
		private System.Windows.Forms.BindingSource bindingSource1;
		private SpecialPictureBox ImageBox;
	}
}


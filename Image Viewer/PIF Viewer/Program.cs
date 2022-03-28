using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace PIF_Viewer
{
	internal static class Program
	{
		/// <summary>
		/// Der Haupteinstiegspunkt für die Anwendung.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(args.Length == 0 ? new ViewerForm(string.Empty) : new ViewerForm(args[0]));
		}
	}
}

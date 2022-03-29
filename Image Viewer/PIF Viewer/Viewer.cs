using System;
using System.IO;
using System.Drawing;
using System.Windows.Forms;

namespace PIF_Viewer
{
	public partial class ViewerForm : Form
	{
		// Bitmap to the PIF Image
		private Bitmap PIFbitmap;
		// Name of the PIF Image
		private string PIFName;

		public ViewerForm(string path)
		{
			InitializeComponent();

			if (path != string.Empty && Path.GetExtension(path).ToLower() == ".pif")
			{
				// Load Image directly
				LoadPIFImage(path);
			}
		}

		private void Menu_Open_Click(object sender, EventArgs e)
		{
			using (OpenFileDialog ofd = new OpenFileDialog())
			{
				ofd.Filter = "PIF Image Files (*.pif)|*.pif";
				ofd.Multiselect = false;
				ofd.RestoreDirectory = true;

				if (ofd.ShowDialog() == DialogResult.OK)
				{
					PIFName = ofd.SafeFileName;
					LoadPIFImage(ofd.FileName);
				}
			}
		}

		private void Menu_Export_Click(object sender, EventArgs e)
		{
			if (PIFbitmap == null) return;

			using (SaveFileDialog sfd = new SaveFileDialog())
			{
				sfd.Title = "Export PIF Image";
				sfd.Filter = "Bitmap Image|*.bmp";
				sfd.FileName = PIFName.Substring(0, PIFName.Length - 4);
				if (sfd.ShowDialog() == DialogResult.OK)
				{
					PIFbitmap.Save(sfd.FileName);
				}				
			}
		}

		private void Menu_Close_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}
		
		void LoadPIFImage(string PIFpath)
		{
			var PIFname = string.Empty;
			UInt32 ImageOffset, ImageDataSize, ImageDataPointer = 0;
			UInt16 ImageType, BitsPerPixel, ImageWidth, ImageHeight, ColorTableSize, Compression, ColorTableColors = 0;
			byte[] ImageDataRaw;
			var PIFbinary = new byte[0];
			var ColorTable = new byte[0];

			// Open the File
			using (FileStream fs = new FileStream(PIFpath, FileMode.Open, FileAccess.Read))
			{
				// Read in the whole file
				PIFbinary = new byte[(int)fs.Length];
				fs.Read(PIFbinary, 0, (int)fs.Length);
			}

			if (PIFbinary.Length < 0x1D)
			{
				MessageBox.Show("Invalid PIF File", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			if (!(PIFbinary[0] == 'P' && PIFbinary[1] == 'I' && PIFbinary[2] == 'F' && PIFbinary[3] == '\0'))
			{
				MessageBox.Show("This is not a valid PIF file.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			this.Text = String.Format("PIF Image Viewer - {0}", PIFName);

			ImageOffset = ReadUINT32(PIFbinary, PIFFormat.OFFSETImageOffset);

			ImageType = ReadUINT16(PIFbinary, PIFFormat.OFFSETImageType);
			BitsPerPixel = ReadUINT16(PIFbinary, PIFFormat.OFFSETBitsperPixel);
			ImageWidth = ReadUINT16(PIFbinary, PIFFormat.OFFSETImageWidth);
			ImageHeight = ReadUINT16(PIFbinary, PIFFormat.OFFSETImageHeight);
			ImageDataSize = ReadUINT32(PIFbinary, PIFFormat.OFFSETImageSize);
			ColorTableSize = ReadUINT16(PIFbinary, PIFFormat.OFFSETColorTableSize);
			Compression = ReadUINT16(PIFbinary, PIFFormat.OFFSETCompression);

			// Create the Bitmap
			PIFbitmap = new Bitmap(ImageWidth, ImageHeight, System.Drawing.Imaging.PixelFormat.Format24bppRgb);

			// Prepare the raw image data buffer
			ImageDataRaw = new byte[ImageDataSize];

			Array.Copy(PIFbinary, ImageOffset, ImageDataRaw, 0, ImageDataSize);

			// Decompress if RLE and fill the ImageData Buffer with it
			if (Compression == PIFFormat.Compression)
			{
				ImageDataRaw = DecompressRLE(ImageDataRaw, ImageDataSize, BitsPerPixel, ImageHeight * ImageWidth);
			}

			if ((ImageType != PIFFormat.ImageTypeRGB888) && ((ImageType & 0xFF00) != PIFFormat.ImageTypeINDEX))
			{
				// Convert non RGB888 image data to RGB888
				ImageDataRaw = ConvertToRGB888(ImageDataRaw, ImageType, (UInt32)ImageHeight * ImageWidth);
			}
			else if ((ImageType & 0xFF00) == PIFFormat.ImageTypeINDEX)
			{
				// Note the amount of colors, depending on the selected indexed format
				if (ImageType == PIFFormat.ImageTypeIND24)
				{
					ColorTableColors = Convert.ToUInt16(ColorTableSize / 3);
				}
				else if (ImageType == PIFFormat.ImageTypeIND16)
				{
					ColorTableColors = Convert.ToUInt16(ColorTableSize / 2);
				}
				else
				{
					ColorTableColors = ColorTableSize;
				}

				// Indexed Image, contains a color table
				ColorTable = new byte[ColorTableSize];

				for (UInt16 i = 0; i < ColorTableSize; i++)
				{
					ColorTable[i] = PIFbinary[i + PIFFormat.ColorTableOffset];
				}

				if (ImageType != PIFFormat.ImageTypeIND24)
				{
					// Reuse the ConvertToRGB888 function to convert the ColorTable to RGB888
					ColorTable = ConvertToRGB888(ColorTable, (ImageType == PIFFormat.ImageTypeIND16) ? PIFFormat.ImageTypeRGB565 : PIFFormat.ImageTypeRGB332, ColorTableColors);
				}
				// Convert indexed image data to raw RGB888
				ImageDataRaw = ConvertIndexedToRGB888(ImageDataRaw, ImageType, ColorTable, BitsPerPixel, (UInt32)ImageHeight * ImageWidth);
			}

			for (int imgH = 0; imgH < ImageHeight; imgH++)
			{
				for (int imgW = 0; imgW < ImageWidth; imgW++)
				{
					PIFbitmap.SetPixel(imgW, imgH, Color.FromArgb(ImageDataRaw[ImageDataPointer + 2], ImageDataRaw[ImageDataPointer + 1], ImageDataRaw[ImageDataPointer]));
					ImageDataPointer += 3;
				}
			}
			ImageBox.Image = PIFbitmap;

			// Last but not least, set the text of the status labels with the image information
			switch(ImageType)
			{
				case PIFFormat.ImageTypeRGB888:
					ImageType_LBL.Text = "RGB888";
					break;
				case PIFFormat.ImageTypeRGB565:
					ImageType_LBL.Text = "RGB565";
					break;
				case PIFFormat.ImageTypeRGB332:
					ImageType_LBL.Text = "RGB332";
					break;
				case PIFFormat.ImageTypeRGB16C:
					ImageType_LBL.Text = "RGB16C";
					break;
				case PIFFormat.ImageTypeBLWH:
					ImageType_LBL.Text = "B/W";
					break;
				case PIFFormat.ImageTypeIND24:
					ImageType_LBL.Text = "IND24";
					break;
				case PIFFormat.ImageTypeIND16:
					ImageType_LBL.Text = "IND16";
					break;
				case PIFFormat.ImageTypeIND8:
					ImageType_LBL.Text = "IND8";
					break;
			}			
			FileSize_LBL.Text = String.Format("{0} Bytes", ReadUINT32(PIFbinary, PIFFormat.OFFSETFileSize));
			ImageSize_LBL.Text = String.Format("{0}, {1}", ImageWidth, ImageHeight);
			DataSize_LBL.Text = String.Format("{0} Bytes {1}", ImageDataSize, (Compression == PIFFormat.Compression) ? "RLE" : "");
			ColorTable_LBL.Text = String.Format("{0} Colors / {1} Bytes", ColorTableColors, ColorTableSize);
		}

		// Convert RGB565, RGB332, RGB16C and B/W images to RGB888
		byte[] ConvertToRGB888(byte[] nonRGB888_ImageData, UInt16 ImageType, UInt32 imageSize)
		{
			byte[] RGB888 = new byte[imageSize * 3];

            for (int i = 0; i < RGB888.Length; i++)
            {
                RGB888[i] = 0;
            }

			UInt32 ImageDataPointer = 0;

			if (ImageType == PIFFormat.ImageTypeRGB565)
			{
				for (int i = 0; i < nonRGB888_ImageData.Length; i += 2)
				{
					RGB888[ImageDataPointer] = (byte)Math.Round((double)((nonRGB888_ImageData[i] & 0x1F) << 3) * 1.028225806451613);	// Blue
					RGB888[ImageDataPointer + 1] = (byte)Math.Round((double)(((nonRGB888_ImageData[i] & 0xE0) >> 3) | ((nonRGB888_ImageData[i + 1] & 0x07) << 5)) * 1.011904762);	// Green
					RGB888[ImageDataPointer + 2] = (byte)Math.Round((double)(nonRGB888_ImageData[i + 1] & 0xF8) * 1.028225806451613);	// Red
					ImageDataPointer += 3;
				}
			}
			else if (ImageType == PIFFormat.ImageTypeRGB332)
			{
				for (int i = 0; i < nonRGB888_ImageData.Length; i++)
				{
					RGB888[ImageDataPointer] = (byte)Math.Round((double)((nonRGB888_ImageData[i] & 0x03) << 6) * 1.328125);			// Blue
					RGB888[ImageDataPointer + 1] = (byte)Math.Round((double)((nonRGB888_ImageData[i] & 0x1C) << 3) * 1.138392857);	// Green
					RGB888[ImageDataPointer + 2] = (byte)Math.Round((double)(nonRGB888_ImageData[i] & 0xE0) * 1.138392857);			// Red
					ImageDataPointer += 3;
				}
			}
			else if (ImageType == PIFFormat.ImageTypeRGB16C)
			{
				for (int i = 0; i < imageSize; i++)
				{
					int ColorNumber = (nonRGB888_ImageData[i / 2] & (0x0F << ((i % 2) * 4))) >> ((i % 2) * 4);
					RGB888[ImageDataPointer] = (byte)Math.Round((double)(255 * (2.0 / 3.0 * (ColorNumber & 1) / 1.0 + 1.0 / 3.0 * (ColorNumber & 8) / 8.0)));		// Blue
					RGB888[ImageDataPointer + 1] = (byte)Math.Round((double)(255 * (2.0 / 3.0 * (ColorNumber & 2) / 2.0 + 1.0 / 3.0 * (ColorNumber & 8) / 8.0)));	// Green
					RGB888[ImageDataPointer + 2] = (byte)Math.Round((double)(255 * (2.0 / 3.0 * (ColorNumber & 4) / 4.0 + 1.0 / 3.0 * (ColorNumber & 8) / 8.0)));	// Red
					ImageDataPointer += 3;
				}
			}
            else if (ImageType == PIFFormat.ImageTypeBLWH)
            {
                for (int i = 0; i < imageSize; i++)
                {
                    int val = (nonRGB888_ImageData[i / 8] & (1<< (i % 8)));
                    if (val != 0)
                    {
                        RGB888[ImageDataPointer] = 0xFF;
                        RGB888[ImageDataPointer + 1] = 0xFF;
                        RGB888[ImageDataPointer + 2] = 0xFF;
                    }
                    ImageDataPointer += 3;
                }
            }
			return RGB888;
		}

		byte[] ConvertIndexedToRGB888(byte[] indexedImageData, UInt16 ImageType, byte[] ColorTableRGB888, UInt16 BitsPerPixel, UInt32 ImageSize)
		{
			byte[] RGB888 = new byte[ImageSize * 3];
			UInt32 ImageDataPointer = 0;
			for (int i = 0; i < RGB888.Length; i++)
			{
				RGB888[i] = 0;
			}

			if (BitsPerPixel > 4)
			{
				for (int i = 0; i < indexedImageData.Length; i++)
				{
					RGB888[ImageDataPointer] = ColorTableRGB888[indexedImageData[i] * 3];
					RGB888[ImageDataPointer + 1] = ColorTableRGB888[indexedImageData[i] * 3 + 1];
					RGB888[ImageDataPointer + 2] = ColorTableRGB888[indexedImageData[i] * 3 + 2];
					ImageDataPointer += 3;
				}
			}
			else if (BitsPerPixel >= 3)
			{
				for (int i = 0; i < ImageSize; i++)
				{
					int indexedCol = (indexedImageData[i / 2] & (0x0F << ((i % 2) * 4))) >> ((i % 2) * 4);
					RGB888[ImageDataPointer] = ColorTableRGB888[indexedCol * 3];
					RGB888[ImageDataPointer + 1] = ColorTableRGB888[indexedCol * 3 + 1];
					RGB888[ImageDataPointer + 2] = ColorTableRGB888[indexedCol * 3 + 2];
					ImageDataPointer += 3;
				}
			}
			else if (BitsPerPixel == 2)
			{
				for (int i = 0; i < ImageSize; i++)
				{
					int indexedCol = (indexedImageData[i / 4] & (3 << ((i % 4) * 2)) >> ((i % 4) * 2));
					RGB888[ImageDataPointer] = ColorTableRGB888[indexedCol * 3];
					RGB888[ImageDataPointer + 1] = ColorTableRGB888[indexedCol * 3 + 1];
					RGB888[ImageDataPointer + 2] = ColorTableRGB888[indexedCol * 3 + 2];
					ImageDataPointer += 3;
				}
			}
			else if (BitsPerPixel == 1)
			{
				for (int i = 0; i < ImageSize; i++)
				{
					int indexedCol = (indexedImageData[i / 8] & (1 << (i % 8)) >> (i % 8));
					RGB888[ImageDataPointer] = ColorTableRGB888[indexedCol * 3];
					RGB888[ImageDataPointer + 1] = ColorTableRGB888[indexedCol * 3 + 1];
					RGB888[ImageDataPointer + 2] = ColorTableRGB888[indexedCol * 3 + 2];
					ImageDataPointer += 3;
				}
			}

			return RGB888;
		}

		byte[] DecompressRLE(byte[] compressedData, UInt32 length, int BitsPerPixel, int finalImageResolution)
		{
			byte[] uncompressedData = new byte[finalImageResolution];
			sbyte rleInst = 0;
			byte[] imageData = new byte[3];
			UInt32 dataCounter;
			UInt32 ImagePointer = 0;

			if (BitsPerPixel == 24)
			{
				uncompressedData = new byte[finalImageResolution * 3];
			}
			else if (BitsPerPixel == 16)
			{
				uncompressedData = new byte[finalImageResolution * 2];
			}
            else if (BitsPerPixel == 8)
            {
                uncompressedData = new byte[finalImageResolution];
            }
            else if (BitsPerPixel == 4)
            {
                uncompressedData = new byte[finalImageResolution / 2];
            }
            

			for (UInt32 i = 0; i < uncompressedData.Length; i++)
			{
				uncompressedData[i] = 0x7F;
			}

			for (dataCounter = 0; dataCounter < length; dataCounter++)
			{
				// Load next RLE-compressed Image Data
				imageData[0] = compressedData[dataCounter];

				// Check the RLE instruction
				if (rleInst > 0)
				{
					if (BitsPerPixel >= 16)
					{
						imageData[1] = compressedData[++dataCounter];
					}
					if (BitsPerPixel == 24)
					{
						imageData[2] = compressedData[++dataCounter];
					}
					// RLE Instruction is positive: Repeat the image Data rleInst amount of times
					for (; rleInst > 0; rleInst--)
					{
						uncompressedData[ImagePointer++] = imageData[0];
						if (BitsPerPixel >= 16)
						{
							uncompressedData[ImagePointer++] = imageData[1];
						}
						if (BitsPerPixel == 24)
						{
							uncompressedData[ImagePointer++] = imageData[2];
						}
					}
				}
				else if (rleInst < 0)
				{
					// RLE Instruction is negative: The next (rleInst * -1) amount of image data is uncompressed
					uncompressedData[ImagePointer++] = imageData[0];
					if (BitsPerPixel >= 16)
					{
						uncompressedData[ImagePointer++] = compressedData[++dataCounter];
					}
					if (BitsPerPixel == 24)
					{
						uncompressedData[ImagePointer++] = compressedData[++dataCounter];
					}
					rleInst++;
				}
				else
				{
					// RLE Instruction is Zero? Load next RLE Instruction
					rleInst = (sbyte)imageData[0];
				}
			}

			return uncompressedData;
		}

		/* Helper functions to enforce little-endian reading */
		UInt16 ReadUINT16(byte[] fileBytes, UInt32 offset)
		{
			UInt16 number;

			number = (UInt16)(fileBytes[0 + offset] | fileBytes[1 + offset] << 8);

			return number;
		}

		UInt32 ReadUINT32(byte[] fileBytes, UInt32 offset)
		{
			UInt32 number;

			number = (UInt32)(fileBytes[0 + offset] | fileBytes[1 + offset] << 8 | fileBytes[2 + offset] << 16 | fileBytes[3 + offset] << 24);

			return number;
		}	
	}

	public class SpecialPictureBox : PictureBox
	{
		protected override void OnPaint(PaintEventArgs pe)
		{
			pe.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
			pe.Graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.Half;
			pe.Graphics.CompositingQuality = System.Drawing.Drawing2D.CompositingQuality.AssumeLinear;
			pe.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.None;
			base.OnPaint(pe);
		}
	}

	static class PIFFormat
	{
		public const int OFFSETSignature	= 0x00;
		public const int OFFSETFileSize		= 0x04;
		public const int OFFSETImageOffset	= 0x08;
		public const int OFFSETImageType	= 0x0C;
		public const int OFFSETBitsperPixel	= 0x0E;
		public const int OFFSETImageWidth	= 0x10;
		public const int OFFSETImageHeight	= 0x12;
		public const int OFFSETImageSize	= 0x14;
		public const int OFFSETColorTableSize = 0x18;
		public const int OFFSETCompression	= 0x1A;
		public const int OFFSETColorTable	= 0x1C;

		public const UInt16 ImageTypeRGB888 = 0x433C;
		public const UInt16 ImageTypeRGB565 = 0xe5c5;
		public const UInt16 ImageTypeRGB332 = 0x1e53;
		public const UInt16 ImageTypeRGB16C = 0xb895;
		public const UInt16 ImageTypeBLWH	= 0x7DAA;
		public const UInt16 ImageTypeIND24	= 0x4952;
		public const UInt16 ImageTypeIND16	= 0x4947;
		public const UInt16 ImageTypeIND8	= 0x4942;
		public const UInt16 ImageTypeINDEX  = 0x4900;
		public const UInt16 ColorTableOffset = 28;
		public const UInt16 Compression		= 0x7DDE;
	}
}

using System;
using System.IO;
using System.Drawing;
using System.Windows.Forms;

namespace PIF_Viewer
{
	public partial class ViewerForm : Form
	{
		public ViewerForm()
		{
			InitializeComponent();
		}
		private Bitmap PIFbitmap;
		private void Menu_Open_Click(object sender, EventArgs e)
		{
			var PIFpath = string.Empty;
			var PIFname = string.Empty;
			UInt32 ImageOffset, ImageDataSize, ImageDataPointer = 0;
			UInt16 ImageType, BitsPerPixel, ImageWidth, ImageHeight, ColorTableSize, Compression, ColorTableColors;
			byte[] ImageDataRaw;

			using (OpenFileDialog ofd = new OpenFileDialog())
			{
				ofd.Filter = "PIF Image Files (*.pif)|*.pif";
				ofd.Multiselect = false;
				ofd.RestoreDirectory = true;

				if (ofd.ShowDialog() == DialogResult.OK)
				{
					PIFpath = ofd.FileName;
					PIFname = ofd.SafeFileName;

					using (FileStream fs = new FileStream(PIFpath, FileMode.Open, FileAccess.Read))
					{
						var len = (int)fs.Length;
						var PIFbinary = new byte[len];
						fs.Read(PIFbinary, 0, len);

						if (PIFbinary[0] == 'P' && PIFbinary[1] == 'I' && PIFbinary[2] == 'F' && PIFbinary[3] == '\0')
						{
							this.Text = String.Format("PIF Image Viewer - {0}", PIFname);

							ImageOffset = ReadUINT32(PIFbinary, 8);

							ImageType = ReadUINT16(PIFbinary, 12);
							BitsPerPixel = ReadUINT16(PIFbinary, 14);
							ImageWidth = ReadUINT16(PIFbinary, 16);
							ImageHeight = ReadUINT16(PIFbinary, 18);
							ImageDataSize = ReadUINT32(PIFbinary, 20);
							ColorTableSize = ReadUINT16(PIFbinary, 24);
							Compression = ReadUINT16(PIFbinary, 26);

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
							
							// If there is a color table, buffer it
							if ((ImageType & 0xFF00) == 0x4900)
							{
								// Indexed Image, contains a color table
								var ColorTable = new byte[ColorTableSize];

								for (UInt16 i = 0; i < ColorTableSize; i++)
								{
									ColorTable[i] = PIFbinary[i + PIFFormat.ColorTableOffset];
								}
							}

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

							switch(ImageType)
							{
								case PIFFormat.ImageTypeIND8:
									break;
								case PIFFormat.ImageTypeIND16:
									break;
								case PIFFormat.ImageTypeIND24:
									break;
								case PIFFormat.ImageTypeBLWH:
									break;
								case PIFFormat.ImageTypeRGB16C:
									break;
								case PIFFormat.ImageTypeRGB332:
									break;
								case PIFFormat.ImageTypeRGB565:
									break;
								case PIFFormat.ImageTypeRGB888:
									for (int imgH = 0; imgH < ImageHeight; imgH++)
									{
										for (int imgW = 0; imgW < ImageWidth; imgW++)
										{
											PIFbitmap.SetPixel(imgW, imgH, Color.FromArgb(ImageDataRaw[ImageDataPointer + 2], ImageDataRaw[ImageDataPointer + 1], ImageDataRaw[ImageDataPointer]));
											ImageDataPointer += 3;
										}
									}
									ImageBox.Image = PIFbitmap;
									break;
								default:
									MessageBox.Show("Unknown Image Mode!");
									return;
							}
							
							FileSize_LBL.Text = String.Format("{0} Bytes", ReadUINT32(PIFbinary, 4));
							ImageSize_LBL.Text = String.Format("{0}, {1}", ImageWidth, ImageHeight);
							DataSize_LBL.Text = String.Format("{0} Bytes", ImageDataSize);
							ColorTable_LBL.Text = String.Format("{0} Colors / {1} Bytes", ColorTableColors, ColorTableSize);
						}
					}
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
		/*
		byte[] DecompressRLE(byte[] compressedData, UInt32 length, int BitsPerPixel, int finalImageResolution)
		{
			byte[] uncompressedData = new byte[0];
			sbyte rleInst = 0;
			byte[] imageData = new byte[3];
			UInt32 dataCounter;

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
						uncompressedData = AddByte(uncompressedData, imageData[0]);
						if (BitsPerPixel >= 16)
						{
							uncompressedData = AddByte(uncompressedData, imageData[1]);
						}
						if (BitsPerPixel == 24)
						{
							uncompressedData = AddByte(uncompressedData, imageData[2]);
						}
					}
				}
				else if (rleInst < 0)
				{
					// RLE Instruction is negative: The next (rleInst * -1) amount of image data is uncompressed
					uncompressedData = AddByte(uncompressedData, imageData[0]);
					if (BitsPerPixel >= 16)
					{
						uncompressedData = AddByte(uncompressedData, compressedData[++dataCounter]);
					}
					if (BitsPerPixel == 24)
					{
						uncompressedData = AddByte(uncompressedData, compressedData[++dataCounter]);
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
		
		byte[] AddByte(byte[] bArray, byte newByte)
		{
			byte[] newArray = new byte[bArray.Length + 1];
			bArray.CopyTo(newArray, 0);
			newArray[bArray.Length] = newByte;
			return newArray;
		}
		*/
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
			/*
			pe.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
			pe.Graphics.CompositingQuality = System.Drawing.Drawing2D.CompositingQuality.AssumeLinear;
			pe.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.None;
			*/
			base.OnPaint(pe);
		}
	}

	static class PIFFormat
	{
		public const UInt16 ImageTypeRGB888 = 0x433C;
		public const UInt16 ImageTypeRGB565 = 0xe5c5;
		public const UInt16 ImageTypeRGB332 = 0x1e53;
		public const UInt16 ImageTypeRGB16C = 0xb895;
		public const UInt16 ImageTypeBLWH	= 0x7DAA;
		public const UInt16 ImageTypeIND24	= 0x4952;
		public const UInt16 ImageTypeIND16	= 0x4947;
		public const UInt16 ImageTypeIND8	= 0x4942;
		public const UInt16 ColorTableOffset = 28;
		public const UInt16 Compression		= 0x7DDE;
	}
}

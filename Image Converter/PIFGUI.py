# PIF GUI Converter by gfcwfzkm (02.2022)
# Dependencies: pillow & pysimplegui
# Might require python 3.10 or higher
# (Sorry if things don't look too professional, functionality first, optimizations later)

import io
import threading
from enum import Enum
import PySimpleGUI as sg	# pip install pysimplegui
import PIL.Image			# pip install pillow
import PIL.ImageColor

THREADWAIT = 0.1

# CGA / 16 Color palette generated with the following formula:
# red   = 2/3×(colorNumber & 4)/4 + 1/3×(colorNumber & 8)/8 * 255
# green = 2/3×(colorNumber & 2)/2 + 1/3×(colorNumber & 8)/8 * 255
# blue  = 2/3×(colorNumber & 1)/1 + 1/3×(colorNumber & 8)/8 * 255
WINDOWS16COLORPAL = [
	0, 0, 0,		# black
	0, 0, 170,		# blue
	0, 170, 0,		# green
	0, 170, 170,	# cyan
	170, 0, 0,		# red
	170, 0, 170,	# magenta
	170, 170, 0,	# dark yellow
	170, 170, 170,	# light gray
	85, 85, 85,		# dark gray
	85, 85, 255,	# light blue
	85, 255, 85,	# light green
	85, 255, 255,	# light cyan
	255, 85, 85,	# light red
	255, 85, 255,	# light magenta
	255, 255, 85,	# yellow
	255, 255, 255	# white
]

class ConversionType(Enum):
	UNDEFINED	= 0
	INDEXED332	= 1
	INDEXED565	= 2
	INDEXED888	= 3
	MONOCHROME 	= 4
	RGB16C		= 5
	RGB332 		= 6
	RGB565 		= 7
	RGB888 		= 8

class CompressionType(Enum):
	NO_COMPRESSION 	= 0
	RLE_COMPRESSION = 1

#########################################################################
#                     Convert Image to Bytes                            #
#########################################################################
def convToBytes(image, resize=None):
	img = image.copy()
	
	cur_width, cur_height = img.size
	if resize:
		new_width, new_height = resize
		scale = min(new_height/cur_height, new_width/cur_width)
		img = img.resize((int(cur_width*scale), int(cur_height*scale)), PIL.Image.BILINEAR)
	
	ImgBytes = io.BytesIO()
	img.save(ImgBytes, format="PNG")
	del img
	return ImgBytes.getvalue()

#########################################################################
#                            Resize Image                               #
#########################################################################
def resizeImage(window, image, offsets):
	oldWindowSize = window.size
	# Resize the image based on the free space available for the [image] element by substracting the offsets from the window size
	# (Basically avoiding to check the [image]-element's size, trying to save calls here by working with stored variables)
	window['-IMGBOX-'].update(convToBytes(image, (oldWindowSize[0] - offsets[0], oldWindowSize[1] - offsets[1])))

#########################################################################
#                          Convert the Image                            #
#########################################################################
def convertImage(image, resize, conversion, ToDisplay, colLen, colArray, dithering):
	ImageToProcess = image.resize(tuple(resize),PIL.Image.LANCZOS)
	ImageToReturn = image.copy()
	match conversion:
		case ConversionType.INDEXED332:
			palImage = PIL.Image.new('P', (16,16))
			tempColList = []
			for entries in range(colLen):
				if (ToDisplay == True):
					tempColList.append(round(colArray[entries * 3] * 1.138392857))		# RED 3
					tempColList.append(round(colArray[entries * 3 + 1] * 1.138392857))	# GREEN 3
					tempColList.append(round(colArray[entries * 3 + 2] * 1.328125))	# BLUE 2
				else:
					tempColList.append(colArray[entries * 3] & 0b11100000)		# RED 3
					tempColList.append(colArray[entries * 3 + 1] & 0b11100000)	# GREEN 3
					tempColList.append(colArray[entries * 3 + 2] & 0b11000000)	# BLUE 2
			tempColList.extend(tempColList[:3] * (256 - colLen))
			palImage.putpalette(tempColList)
			ImageToReturn = ImageToProcess.quantize(colLen, palette=palImage, dither=dithering)
		case ConversionType.INDEXED565:
			palImage = PIL.Image.new('P', (16,16))
			tempColList = []
			for entries in range(colLen):
				if (ToDisplay == True):
					tempColList.append(round(colArray[entries * 3] * 1.028225806))	# RED 5
					tempColList.append(round(colArray[entries * 3 + 1] * 1.011904762))	# GREEN 6
					tempColList.append(round(colArray[entries * 3 + 2] * 1.028225806))	# BLUE 5
				else:
					tempColList.append(colArray[entries * 3] & 0b11111000)	# RED 5
					tempColList.append(colArray[entries * 3 + 1] & 0b11111100)	# GREEN 6
					tempColList.append(colArray[entries * 3 + 2] & 0b11111000)	# BLUE 5
			tempColList.extend(tempColList[:3] * (256 - colLen))
			palImage.putpalette(tempColList)
			ImageToReturn = ImageToProcess.quantize(colLen, palette=palImage, dither=dithering)
		case ConversionType.INDEXED888:
			palImage = PIL.Image.new('P', (16,16))
			tempColList = []
			for entries in range(colLen):
				tempColList.append(colArray[entries * 3])		# RED
				tempColList.append(colArray[entries * 3 + 1])	# GREEN
				tempColList.append(colArray[entries * 3 + 2])	# BLUE
			tempColList.extend(tempColList[:3] * (256 - colLen))
			palImage.putpalette(tempColList)
			ImageToReturn = ImageToProcess.quantize(colLen, palette=palImage, dither=dithering)
		case ConversionType.MONOCHROME:
			ImageToReturn = ImageToProcess.convert("1", dither=dithering)
		case ConversionType.RGB16C:
			palImage = PIL.Image.new('P', (16,16))
			palList = WINDOWS16COLORPAL.copy()
			palList.extend(palList[:3] * 240)
			palImage.putpalette(palList)
			ImageToReturn = ImageToProcess.quantize(16, palette=palImage, dither=dithering)
		case ConversionType.RGB332:
			palImage = PIL.Image.new('P', (16,16))
			palList = []
			for colors in range(256):
				if (ToDisplay == True):
					palList.append(round((colors & 0b11100000) * 1.138392857))			# RED 3
					palList.append(round(((colors & 0b00011100) << 3) * 1.138392857))	# GREEN 3
					palList.append(round(((colors & 0b00000011) << 6) * 1.328125))		# BLUE 2
				else:
					palList.append(colors & 0b11100000)			# RED 5
					palList.append((colors & 0b00011100) << 3)	# GREEN 6
					palList.append((colors & 0b00000011) << 6)	# BLUE 5
			palImage.putpalette(palList)
			ImageToReturn = ImageToProcess.quantize(256, palette=palImage, dither=dithering)
		case ConversionType.RGB565:
			ImageToReturn = ImageToProcess.copy()
			for x in range(ImageToReturn.width):
				for y in range(ImageToReturn.height):
					color = list(ImageToReturn.getpixel((x,y)))
					if (ToDisplay == True):
						color[0] = round((color[0] & 0xF8) * 1.028225806)
						color[1] = round((color[1] & 0xFC) * 1.011904762)
						color[2] = round((color[2] & 0xF8) * 1.028225806)
					else:
						color[0] = color[0] & 0xF8
						color[1] = color[1] & 0xFC
						color[2] = color[2] & 0xF8
					ImageToReturn.putpixel((x,y), tuple(color))
		case ConversionType.RGB888:
			ImageToReturn = ImageToProcess.copy()
	del ImageToProcess
	return ImageToReturn

def rle_compression(pixelArray):
	rlePosition = []
	compressedArray = []
	tempArray = [0] * 129
	tempIndex = 0
	pixelIndex = 0
	compressedIndex = 0

	tempArray[0] = pixelArray[pixelIndex]
	pixelIndex += 1

	while (pixelIndex < len(pixelArray)):

		tempArray[1] = pixelArray[pixelIndex]
		pixelIndex += 1

		if (tempArray[1] == tempArray[0]):
			# compressible
			tempIndex = 2
			while True:
				tempArray[tempIndex] = pixelArray[pixelIndex]
				tempIndex += 1
				pixelIndex += 1
				if (tempArray[tempIndex] != tempArray[tempIndex - 1]) or (pixelIndex >= len(pixelArray)) or (tempIndex >= 127 ):
					break
			
			compressedArray.append(tempIndex - 1)
			compressedArray.append(tempArray[0])

			tempArray[0] = tempArray[tempIndex - 1]
		else:
			# Not compressible
			print('x')

	return 0

#########################################################################
#                           RLE Compression                             #
#########################################################################
def rle_compress(pixelArray):
	outlist = []
	rlePos = []
	tempArray = [None]*129
	outCnt = 0
	pixCnt = 0

	tempArray[0] = pixelArray[pixCnt]
	pixCnt += 1

	while not (pixCnt > len(pixelArray)):

		if (tempArray[0] == None):	break
		if (pixCnt >= len(pixelArray)):
			tempArray[1] = None
		else:
			tempArray[1] = pixelArray[pixCnt]
		pixCnt += 1

		# Check if a compressible sequence starts or not
		if (tempArray[0] != tempArray[1]):
			# Uncompressible sequence!
			outCnt = 1
		
			# Run this loop as long as there is still data to read, the counter being below 128 AND 
			# the previous read word isn't the same as the one before
			while True:
				outCnt += 1
				if (pixCnt >= len(pixelArray)):
					tempArray[outCnt] = None
				else:
					tempArray[outCnt] = pixelArray[pixCnt]
				pixCnt += 1
				# Makeshift "Do-While" loop
				if (pixCnt >= len(pixelArray)):
					tempArray[outCnt] = None
					break
				if not (outCnt < 127):
					break
				if not (tempArray[outCnt] != tempArray[outCnt - 1]):
					break

			keep = tempArray[outCnt]
			if (tempArray[outCnt] == tempArray[outCnt - 1]):
				outCnt -= 1
			
			#Add to list
			rlePos.append(len(outlist))
			outlist.append(outCnt * -1)
			outlist.extend(tempArray[:outCnt])
			
			tempArray[0] = tempArray[outCnt]
			if (keep == None or not outCnt < 127):
				continue
		
		# Compressible sequence
		outCnt = 2

		while True:
			if (pixCnt >= len(pixelArray)):
				tempArray[1] = None
			else:
				tempArray[1] = pixelArray[pixCnt]
			pixCnt += 1
			outCnt += 1
			if not (outCnt < 127):
				break
			if not (tempArray[0] == tempArray[1]):
				break
		
		rlePos.append(len(outlist))
		outlist.append(outCnt - 1)
		outlist.append(tempArray[0])

		tempArray[0] = tempArray[1]

	rlePos.append(None)
	return rlePos,outlist

#########################################################################
#               Convert Image & Color Table to PIF                      #
#########################################################################
def convertToPIF(image, resize, conversion, colorLength, colorTable, dithering, compression):
	class ImageH(Enum):
		IMAGETYPE = 0
		BITSPERPIXEL = 1
		IMAGEWIDTH = 2
		IMAGEHEIGHT = 3
		IMAGESIZE = 4
		COLTABLESIZE = 5
		COMPRTYPE = 6
	
	checkImageType = conversion
	imageHeader = [0,0,0,0,0,0,0]
	imageColors = []
	# Image Data contains the pixels in word-size (min 1 byte, max 3 bytes)
	# Proper conversion done when saving the data, BITSPERPIXEL to process correctly
	imageData = []

	imageWord = 0
	imageCnt = 0
	color = 0

	TemporaryImage = image.copy()

	#Write the image header with the information we already have
	imageHeader[ImageH.IMAGEWIDTH.value] = TemporaryImage.width
	imageHeader[ImageH.IMAGEHEIGHT.value] = TemporaryImage.height
	if (compression):
		imageHeader[ImageH.COMPRTYPE.value] = 0x7DDE

	if ((checkImageType != ConversionType.RGB888) and (checkImageType != ConversionType.RGB565)):
		TemporaryImage = convertImage(image, resize, checkImageType, False, colorLength, colorTable, dithering)
	else:
		TemporaryImage = convertImage(image, resize, ConversionType.RGB888, False, colorLength, colorTable, dithering)

	if ((checkImageType == ConversionType.INDEXED332) or (checkImageType == ConversionType.INDEXED565) or (checkImageType == ConversionType.INDEXED888)):
		checkImageType = ConversionType.INDEXED332

	match checkImageType:
		case ConversionType.INDEXED332:
			# Write the Image Type and Bits per Pixel into the imageHeader
			if (conversion == ConversionType.INDEXED332):
				imageHeader[ImageH.IMAGETYPE.value] = 0x4942	# Indexed8 mode selected
			elif (conversion == ConversionType.INDEXED565):
				imageHeader[ImageH.IMAGETYPE.value] = 0x4947	# Indexed16 mode selected
			else:
				imageHeader[ImageH.IMAGETYPE.value] = 0x4952	# Indexed24 mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = int(colorLength - 1).bit_length()

			# Create the color table
			for colval in range(colorLength):
				if (conversion == ConversionType.INDEXED332):
					imageColors.append(colorTable[colval * 3] | (colorTable[colval * 3 + 1] >> 3) | (colorTable[colval * 3 + 2] >> 6))
				elif (conversion == ConversionType.INDEXED565):
					imageColors.append((colorTable[colval * 3] << 8) | (colorTable[colval * 3 + 1] << 3) | (colorTable[colval * 3 + 2] >> 3))
				else:
					imageColors.append((colorTable[colval * 3] << 16) | (colorTable[colval * 3 + 1] << 8) | colorTable[colval * 3 + 2])

			# Write the Image Type and Bits per Pixel into the imageHeader
			if (conversion == ConversionType.INDEXED332):
				imageHeader[ImageH.IMAGETYPE.value] = 0x4942	# Indexed8 mode selected
				imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors)
			elif (conversion == ConversionType.INDEXED565):
				imageHeader[ImageH.IMAGETYPE.value] = 0x4947	# Indexed16 mode selected
				imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors) * 2
			else:
				imageHeader[ImageH.IMAGETYPE.value] = 0x4952	# Indexed24 mode selected
				imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors) * 3
			imageHeader[ImageH.BITSPERPIXEL.value] = int(colorLength - 1).bit_length()
			# Check every pixel and pack it into a byte if possible
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					index = TemporaryImage.getpixel((x,y))
					if (imageHeader[ImageH.BITSPERPIXEL.value] <=4):
						if (imageHeader[ImageH.BITSPERPIXEL.value] >= 3):
							imageWord |= index << imageCnt
							imageCnt += 4
						elif (imageHeader[ImageH.BITSPERPIXEL.value] == 2):
							imageWord |= index << imageCnt
							imageCnt += 2
						elif (imageHeader[ImageH.BITSPERPIXEL.value] == 1):
							imageWord |= index << imageCnt
							imageCnt += 1
						if (imageCnt >= 8):
							imageData.append(imageWord)
							imageWord = 0
							imageCnt = 0
					else:
						imageData.append(index)
			# Add remaining Pixels, if there are any
			if (imageCnt > 0):
				imageData.append(imageWord)					
		case ConversionType.MONOCHROME:
			# Write the Image Type and Bits per Pixel into the imageHeader
			imageHeader[ImageH.IMAGETYPE.value] = 0x7DAA	# B/W mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = 1		# 1 Bits per Pixel
			# Check every pixel and pack to a group of 8 within a byte
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					color = TemporaryImage.getpixel((x,y))
					if (color): # 0 = black, 255 = white
						imageWord |= 1 << imageCnt
					imageCnt += 1
					if (imageCnt >= 8):
						imageData.append(imageWord)
						imageWord = 0
						imageCnt = 0
			# Add remaining Pixels, if there are any
			if (imageCnt > 0):
				imageData.append(imageWord)
		case ConversionType.RGB16C:
			# Write the Image Type and Bits per Pixel into the imageHeader
			imageHeader[ImageH.IMAGETYPE.value] = 0xB895	# RGB16C mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = 4		# 4 Bits per Pixel
			# Check every pixel and pack to a group of 2 within a byte
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					color = TemporaryImage.getpixel((x,y))
					# Check which of the 16 colors is used to return a value between 0 and 15
					imageWord = imageWord | (color << ((imageCnt % 2) * 4))
					imageCnt = imageCnt + 1
					if (imageCnt >= 2):
						imageData.append(imageWord)
						imageWord = 0
						imageCnt = 0
			# Add remaining Pixels, if there are any
			if (imageCnt > 0):
				imageData.append(imageWord)
		case ConversionType.RGB332:
			# Write the Image Type and Bits per Pixel into the imageHeader
			TemporaryImage = TemporaryImage.convert('RGB')
			imageHeader[ImageH.IMAGETYPE.value] = 0x1E53	# RGB332 mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = 8		# 8 Bits per Pixel
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					color = list(TemporaryImage.getpixel((x,y)))
					color[0] = color[0] & 0xE0	# R
					color[1] = color[1] & 0xE0	# G
					color[2] = color[2] & 0xC0	# B
					imageData.append(color[0] | (color[1] >> 3) | (color[2]) >> 6)
		case ConversionType.RGB565:
			imageHeader[ImageH.IMAGETYPE.value] = 0xE5C5	# RGB565 mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = 16		# 16 Bits per Pixel
			# Relatively easy here, just trim the 24-bit RGB image to 16-bit RGB
			# and add it to the list
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					color = list(TemporaryImage.getpixel((x,y)))
					color[0] = color[0] & 0xF8	# R
					color[1] = color[1] & 0xFC	# G
					color[2] = color[2] & 0xF8	# B
					imageData.append((color[0] << 8) | (color[1] << 3) | ((color[2] >> 3)))
		case ConversionType.RGB888:
			imageHeader[ImageH.IMAGETYPE.value] = 0x433C	# RGB888 mode selected
			imageHeader[ImageH.BITSPERPIXEL.value] = 24		# 24 Bits per Pixel
			# Fit the color into 24bit and add it to the list
			for y in range(TemporaryImage.height):
				for x in range(TemporaryImage.width):
					color = list(TemporaryImage.getpixel((x,y)))
					imageData.append((color[0] << 16) | (color[1] << 8) | (color[2]))
	
	# compress the data if requested
	if (compression):
		rlePos, imageData = rle_compress(imageData)
	else:
		rlePos = [None]

	# Write down the image size into the header
	imageHeader[ImageH.IMAGESIZE.value] = len(imageData)

	# Return the image header, color table and image data
	return imageHeader,imageColors,imageData,rlePos

# Python is annoying with that stuff, should have used C#
#########################################################################
#                         Save PIF as binary                            #
#########################################################################
def savePIFbinary(imageHeader, colorTable, imageData, rlePos, path):
	# I'm old school, so I wanna allocate the space before processing the data
	tTotalPIF = []
	tPIFHeader = [None] * 12
	tImgHeader = [None] * 16
	
	if (imageHeader[0] == 0x4942):
		tColTable = [None] * imageHeader[5]
	elif (imageHeader[0] == 0x4947):
		tColTable = [None] * imageHeader[5]
	else:
		tColTable = [None] * imageHeader[5]
	
	tImgData = []

	tPIFHeader[0] = 0x50
	tPIFHeader[1] = 0x49
	tPIFHeader[2] = 0x46
	tPIFHeader[3] = 0
	
	tImgHeader[0] = imageHeader[0] & 0xFF
	tImgHeader[1] = (imageHeader[0] & 0xFF00) >> 8
	tImgHeader[2] = imageHeader[1] & 0xFF
	tImgHeader[3] = (imageHeader[1] & 0xFF00) >> 8
	tImgHeader[4] = imageHeader[2] & 0xFF
	tImgHeader[5] = (imageHeader[2] & 0xFF00) >> 8
	tImgHeader[6] = imageHeader[3] & 0xFF
	tImgHeader[7] = (imageHeader[3] & 0xFF00) >> 8
	tImgHeader[12] = imageHeader[5] & 0xFF
	tImgHeader[13] = (imageHeader[5] & 0xFF00) >> 8
	tImgHeader[14] = imageHeader[6] & 0xFF
	tImgHeader[15] = (imageHeader[6] & 0xFF00) >> 8

	if (imageHeader[5] > 0):
		for index in range(len(colorTable)):
			if (imageHeader[0] == 0x4942):
				tColTable[index] = colorTable[index]
			elif (imageHeader[0] == 0x4947):
				tColTable[index * 2] = (colorTable[index] & 0xFF)
				tColTable[index * 2 + 1] = (colorTable[index] & 0xFF00) >> 8
			else:
				tColTable = [None] * 3 * imageHeader[5]
				tColTable[index * 3] = (colorTable[index] & 0xFF) 
				tColTable[index * 3 + 1] = (colorTable[index] & 0xFF00) >> 8
				tColTable[index * 3 + 2] = (colorTable[index] & 0xFF0000) >> 16

	rleIndex = 0
	for index in range(len(imageData)):
		# RLE Data is always only 1 byte large, while image data can be up to three bytes large
		if ((imageHeader[6] != 0) and (rlePos[rleIndex] == index)):
			rleIndex += 1
			tImgData.append(imageData[index] & 0xFF)
		else:
			if (imageHeader[1] == 16):
				tImgData.append(imageData[index] & 0xFF)
				tImgData.append((imageData[index] & 0xFF00) >> 8)
			elif (imageHeader[1] == 24):
				tImgData.append(imageData[index] & 0xFF)
				tImgData.append((imageData[index] & 0xFF00) >> 8)
				tImgData.append((imageData[index] & 0xFF0000) >> 16)
			else:
				tImgData.append(imageData[index] & 0xFF)

	imageHeader[4] = len(tImgData)

	tImgHeader[8] = imageHeader[4] & 0xFF
	tImgHeader[9] = (imageHeader[4] & 0xFF00) >> 8
	tImgHeader[10] = (imageHeader[4] & 0xFF0000) >> 16
	tImgHeader[11] = (imageHeader[4] & 0xFF000000) >> 24

	tTotalPIF.extend(tPIFHeader)
	tTotalPIF.extend(tImgHeader)
	if (imageHeader[5] > 0):
		tTotalPIF.extend(tColTable)
	iStart = len(tTotalPIF)
	tTotalPIF.extend(tImgData)
	iSize = len(tTotalPIF)

	tTotalPIF[4] = iSize & 0xFF
	tTotalPIF[5] = (iSize & 0xFF00) >> 8
	tTotalPIF[6] = (iSize & 0xFF0000) >> 16
	tTotalPIF[7] = (iSize & 0xFF000000) >> 24

	tTotalPIF[8] = iStart & 0xFF
	tTotalPIF[9] = (iStart & 0xFF00) >> 8
	tTotalPIF[10] = (iStart & 0xFF0000) >> 16
	tTotalPIF[11] = (iStart & 0xFF000000) >> 24

	# write to file
	PIFFile = open(path, "wb")
	PIFFile.write(bytes(tTotalPIF))
	PIFFile.close()

	return iSize

#########################################################################
#                    INDEXING OPTIONS WINDOW                            #
#########################################################################
def get_indexing(image, existingColLen, existingColType, existingColList):
	dfFont = list(sg.DEFAULT_FONT)
	dfFont[1] = 30
	dfFont = tuple(dfFont)

	left_col = [
		[sg.Frame('Lookup Table Color Settings', [
			[sg.Radio('8BPP - RGB332', group_id=10, key='RB_8', enable_events=True)],
			[sg.Radio('16BPP - RGB565', group_id=10, key='RB_16', enable_events=True)],
			[sg.Radio('24BPP - RGB888', group_id=10, key='RB_24', default=True)]
		], expand_x=True)],
		[sg.Frame('Amount of Indexed Colors', [
			[sg.Button('Increment', key='BTN_INC', expand_x=True)],
			[sg.Input('2', size=(4,None), key='TBX_NUM', expand_x=True, enable_events=True)],
			[sg.Button('Decrement', key='BTN_DEC', expand_x=True)]
		],expand_x=True)],
		[sg.VPush()],
		[sg.Button('Analyze Image (RGB888)', key='BTN_AN', expand_x=True)],
		[sg.Button('Apply Changes and Exit', key='BTN_AP', expand_x=True)],
		[sg.Button('Cancel Changes and Exit', key='BTN_CL', expand_x=True)]
	]
	
	layout = [
		[
			sg.Pane([
				sg.Column(left_col, expand_y=True, expand_x=False, element_justification='l')],
				orientation='h', relief=sg.RELIEF_SUNKEN, key='-PANE-'),
			sg.Column([[
					sg.Frame(f'Color Index {val + 1}',[[
						sg.Input('#ffffff', size=(8,None), key=f'C_{val}', disabled=True, enable_events=True),
						sg.Text('⚫', key=f'L_{val}', justification='center', font=dfFont, text_color='#ffffff'),
						sg.ColorChooserButton(f'Color Picker', target=f'C_{val}', bind_return_key=True)]],
					key=f'F_{val}', expand_x=True)] for val in range(256)
			],key='LCOL', expand_x=True, justification='c', scrollable=True, vertical_scroll_only=True)],
	]
	window = sg.Window('Indexing Options', layout, modal=True, finalize=True, size=(500, 600))
	window['-PANE-'].expand(True, True, True)

	IndexingValue = existingColLen
	window['TBX_NUM'].update(IndexingValue)

	for val in range(IndexingValue):
		window[f'C_{val}'].update(value=f'#{existingColList[val * 3]:0{2}x}{existingColList[val * 3 + 1]:0{2}x}{existingColList[val * 3 + 2]:0{2}x}')
		window[f'L_{val}'].update(text_color=window[f'C_{val}'].get())
	
	if (existingColType == ConversionType.INDEXED332):
		window['RB_8'].update(True)
	elif (existingColType == ConversionType.INDEXED565):
		window['RB_16'].update(True)

	while True:
		event, values = window.read()

		if (event=='RB_8' or event=='RB_16'):
			for val in range (IndexingValue):
				inColor = list(PIL.ImageColor.getrgb(window[f'C_{val}'].get()))
				labelColor = [0,0,0]
				if (values['RB_8'] == True):
					inColor[0] = inColor[0] & 0xE0
					inColor[1] = inColor[1] & 0xE0
					inColor[2] = inColor[2] & 0xC0
					labelColor[0] = round(inColor[0] * 1.138392857)
					labelColor[1] = round(inColor[1] * 1.138392857)
					labelColor[2] = round(inColor[2] * 1.328125)
				elif (values['RB_16'] == True):
					inColor[0] = inColor[0] & 0xF8
					inColor[1] = inColor[1] & 0xFC
					inColor[2] = inColor[2] & 0xF8
					labelColor[0] = round(inColor[0] * 1.028225806)
					labelColor[1] = round(inColor[1] * 1.011904762)
					labelColor[2] = round(inColor[2] * 1.028225806)
				else:
					labelColor[0] = inColor[0]
					labelColor[1] = inColor[1]
					labelColor[2] = inColor[2]
				window[f'C_{val}'].update(value=f'#{inColor[0]:0{2}x}{inColor[1]:0{2}x}{inColor[2]:0{2}x}')
				window[f'L_{val}'].update(text_color=f'#{labelColor[0]:0{2}x}{labelColor[1]:0{2}x}{labelColor[2]:0{2}x}')	

		if (event == 'BTN_INC'):
			if (IndexingValue < 255):
				IndexingValue = IndexingValue + 1
				window['TBX_NUM'].update(IndexingValue)

				inColor = list(PIL.ImageColor.getrgb(window[f'C_{IndexingValue - 1}'].get()))
				labelColor = [0,0,0]
				if (values['RB_8'] == True):
					inColor[0] = inColor[0] & 0xE0
					inColor[1] = inColor[1] & 0xE0
					inColor[2] = inColor[2] & 0xC0
					labelColor[0] = round(inColor[0] * 1.138392857)
					labelColor[1] = round(inColor[1] * 1.138392857)
					labelColor[2] = round(inColor[2] * 1.328125)
				elif (values['RB_16'] == True):
					inColor[0] = inColor[0] & 0xF8
					inColor[1] = inColor[1] & 0xFC
					inColor[2] = inColor[2] & 0xF8
					labelColor[0] = round(inColor[0] * 1.028225806)
					labelColor[1] = round(inColor[1] * 1.011904762)
					labelColor[2] = round(inColor[2] * 1.028225806)
				else:
					labelColor[0] = inColor[0]
					labelColor[1] = inColor[1]
					labelColor[2] = inColor[2]
				window[f'C_{IndexingValue - 1}'].update(value=f'#{inColor[0]:0{2}x}{inColor[1]:0{2}x}{inColor[2]:0{2}x}')
				window[f'L_{IndexingValue - 1}'].update(text_color=f'#{labelColor[0]:0{2}x}{labelColor[1]:0{2}x}{labelColor[2]:0{2}x}')	

		if (event == 'BTN_DEC'):
			if (IndexingValue > 2):
				IndexingValue = IndexingValue - 1
				window['TBX_NUM'].update(IndexingValue)

		if (event == 'TBX_NUM'):
			try:
				value = (int)(values['TBX_NUM'])
				if ((value > 255) or (value < 2)):
					value = IndexingValue
				else:
					IndexingValue = value
			except:
				value = IndexingValue
			window['TBX_NUM'].update(value)

		if (event == 'BTN_AP'):
			existingColLen = IndexingValue
			if (values['RB_8'] == True):
				existingColType = ConversionType.INDEXED332
			elif (values['RB_16'] == True):
				existingColType = ConversionType.INDEXED565
			elif (values['RB_24'] == True):
				existingColType = ConversionType.INDEXED888
			for val in range(IndexingValue):
				if (window[f'C_{val}'].get() == 'None'):
					existingColList[val * 3] = 0
					existingColList[val * 3 + 1] = 0
					existingColList[val * 3 + 2] = 0
				else:
					hexColors = PIL.ImageColor.getrgb(window[f'C_{val}'].get())
					existingColList[val * 3] = hexColors[0]
					existingColList[val * 3 + 1] = hexColors[1]
					existingColList[val * 3 + 2] = hexColors[2]
			break

		if (not image == None) and (event == 'BTN_AN'):
			window['RB_24'].update(True)
			imP = image.convert('P', palette=PIL.Image.ADAPTIVE, colors=IndexingValue)
			colTable = imP.getpalette()
			for val in range(IndexingValue):
				window[f'C_{val}'].update(value=f'#{colTable[val * 3]:0{2}x}{colTable[val * 3 + 1]:0{2}x}{colTable[val * 3 + 2]:0{2}x}')
				window[f'L_{val}'].update(text_color=window[f'C_{val}'].get())

		if (isinstance(event, str) and event[0] == 'C' and event[1] == '_'):
			triggeredText = int(event.split('_')[1])
			inColor = list(PIL.ImageColor.getrgb(window[event].get()))
			labelColor = [0,0,0]
			if (values['RB_8'] == True):
				inColor[0] = inColor[0] & 0xE0
				inColor[1] = inColor[1] & 0xE0
				inColor[2] = inColor[2] & 0xC0
				labelColor[0] = round(inColor[0] * 1.138392857)
				labelColor[1] = round(inColor[1] * 1.138392857)
				labelColor[2] = round(inColor[2] * 1.328125)
			elif (values['RB_16'] == True):
				inColor[0] = inColor[0] & 0xF8
				inColor[1] = inColor[1] & 0xFC
				inColor[2] = inColor[2] & 0xF8
				labelColor[0] = round(inColor[0] * 1.028225806)
				labelColor[1] = round(inColor[1] * 1.011904762)
				labelColor[2] = round(inColor[2] * 1.028225806)
			else:
				labelColor[0] = inColor[0]
				labelColor[1] = inColor[1]
				labelColor[2] = inColor[2]
			window[event].update(value=f'#{inColor[0]:0{2}x}{inColor[1]:0{2}x}{inColor[2]:0{2}x}')
			window[f'L_{triggeredText}'].update(text_color=f'#{labelColor[0]:0{2}x}{labelColor[1]:0{2}x}{labelColor[2]:0{2}x}')				

		if (event == sg.WIN_CLOSED or event == 'BTN_CL'):
			break
	window.close()

	return existingColLen, existingColType, existingColList

#########################################################################
#                       FILE SAVED WINDOW                               #
#########################################################################
def file_saved(imageType, compression, size):
	leftCol = [
		[sg.Text("Image Type:")],
		[sg.Text("RLE Compression:")],
		[sg.Text("File size:")]
	]
	rightCol = [
		[sg.Text(imageType)],
		[sg.Text(compression)],
		[sg.Text(f'{size} Bytes')]
	]
	layout = [
		[sg.Text('PIF Image saved!', justification='center', expand_x=True)],
		[sg.Column(leftCol),sg.Column(rightCol)],
		[sg.Button('OK', key='-BTN_OK-', expand_x=True)]
	]
	window = sg.Window("Done!", layout, modal=True)
	while True:
		event, values = window.read()
		if event == "-BTN_OK-" or event == sg.WIN_CLOSED:
			break
	window.close()

#########################################################################
#                            ABOUT WINDOW                               #
#########################################################################
def about():
	layout = [
		[sg.Text('PIF Converter programmed by Pascal G. (alias gfcwfzkm)', justification='center', expand_x=True)],
		[sg.Button('OK', key='-BTN_OK-', expand_x=True)]
	]
	window = sg.Window("About", layout, modal=True)
	while True:
		event, values = window.read()
		if event == "-BTN_OK-" or event == sg.WIN_CLOSED:
			break
	window.close()

#########################################################################
#                            MAIN WINDOW                                #
#########################################################################
def main():
	menubar_layout = [
		['&File', ['&Open','&Save','&Quit']],
		['&Help', ['&About']]
	]

	leftCol_layout = [
		[sg.Frame('Color Settings', [
			[sg.Radio('Indexed', 1, key='-RB_COL_CUS-', enable_events=True), sg.Button('Configure', key='-BTN_CONFIG-', expand_x=True, disabled=False)],
			[sg.Radio('1bpp (Monochrome B/W)', 1, key='-RB_COL_1BM-', enable_events=True)],
			[sg.Radio('4bpp (16 fixed Colors', 1, key='-RB_COL_4C-', enable_events=True)],
			[sg.Radio('8bpp (256 Colors - RGB332)', 1, key='-RB_COL_8B-', enable_events=True)],
			[sg.Radio('16bpp (64k Colors - RGB565)', 1, key='-RB_COL_16B-', enable_events=True)],
			[sg.Radio('24bpp (RGB888)', 1, key='-RB_COL_24B-', enable_events=True, default=True)]],
			expand_x=True)
		],
		[sg.Frame('Compression Type', [
			[sg.Radio('No Compression at all', 2, key='-RB_COMP_NO-', default=True)],
			[sg.Radio('Basic RLE Compression', 2, key='-RB_COMP_RLE-')]],
			expand_x=True)
		],
		[sg.Frame('Dithering Settings', [
			[sg.Radio('No dithering applied', 3, key='-RB_DIT_NO-', default=True)],
			[sg.Radio('Floyd-Steinberg-Dithering', 3, key='-RB_DIT_FS-', disabled=True)]],
			expand_x=True)
		],
		[sg.Frame('Image Resize', [
			[sg.Text('W:'), sg.Input(key='-IN_SIZE_X-', size=(8,1), enable_events=True), sg.Text('H:'), sg.Input(key='-IN_SIZE_Y-', size=(8,1), enable_events=True)],
			[sg.Checkbox('Lock Ratio', key='-CB_RAT-', default=True)]],
			expand_x=True)
		],
		[sg.VPush()],
		[sg.Button('Open', key='-BTN_OPEN-', expand_x=True)],
		[sg.Button('Preview', key='-BTN_PREV-', expand_x=True)],
		[sg.Button('Save', key='-BTN_SAVE-', expand_x=True)]
	]	

	rightCol_layout = [
		[sg.Text(' ', key='-TBXPATH-', expand_x=True, justification='center')],
		[sg.Image(key='-IMGBOX-', expand_x=True, expand_y=True)],
		[sg.Text(size=(80,1))]
	]

	layout = [
		[sg.Menubar(menubar_layout)],
		[sg.Pane([
			sg.Column(leftCol_layout, expand_y=True, expand_x=False, element_justification='l'),
			sg.Column(rightCol_layout, element_justification='c', expand_y=True, expand_x=True)],
			orientation='h', relief=sg.RELIEF_SUNKEN, key='-PANE-')
		]
	]

	window = sg.Window('PIF Image converter', layout, border_depth=0, resizable=True, finalize=True)
	window.bind('<Configure>', "WinEvent")
	window.set_min_size((window.size[0], window.size[1]+30))
	window['-PANE-'].expand(True, True, True)
	window['-IMGBOX-'].expand(True, True)
	window.bring_to_front()

	# Calculate the space between the window and the image control itself. Will be used to set the image's size
	ImageOffset = (window.size[0] - window['-IMGBOX-'].get_size()[0], window.size[1] - window['-IMGBOX-'].get_size()[1])

	OriginalImage = None	# Original Image, do not change
	ImageToDisplay = None	# Image to Display and manipulate
	ColorIndexLength = 2
	ColorIndexType = ConversionType.INDEXED888
	ColorIndexTable = [255] * 3 * 256

	# Setup the threaded timer beforehand, used in TimerStrategy
	timer_windowResize = threading.Timer(THREADWAIT, resizeImage,args=(window, ImageToDisplay, ImageOffset))

	while True:
		events, values = window.read()

		# Quit Application
		if (events == sg.WIN_CLOSED or events == 'Quit'):
			break
			
		# Show preview of the current settings
		if (events == '-BTN_PREV-'):
			if (ImageToDisplay == None): continue
			conType = ConversionType.UNDEFINED
			if values['-RB_COL_CUS-'] == True:	conType = ColorIndexType
			if values['-RB_COL_1BM-'] == True:	conType = ConversionType.MONOCHROME
			if values['-RB_COL_4C-'] == True:	conType = ConversionType.RGB16C
			if values['-RB_COL_8B-'] == True:	conType = ConversionType.RGB332
			if values['-RB_COL_16B-'] == True:	conType = ConversionType.RGB565
			if values['-RB_COL_24B-'] == True:	conType = ConversionType.RGB888

			ImageToDisplay = convertImage(OriginalImage, ((int)(values['-IN_SIZE_X-']),(int)(values['-IN_SIZE_Y-'])), conType, True, ColorIndexLength, ColorIndexTable, values['-RB_DIT_FS-'])
			resizeImage(window, ImageToDisplay, ImageOffset)

		# Open configuration
		if (events == '-BTN_CONFIG-'):
			ColorIndexLength, ColorIndexType, ColorIndexTable = get_indexing(OriginalImage, ColorIndexLength, ColorIndexType, ColorIndexTable)

		# About Window
		if (events == 'About'):
			about()

		# Open  image
		if ((events == 'Open') or (events == '-BTN_OPEN-')):
			filename = sg.popup_get_file('Open Image', no_window=True, show_hidden=False, file_types=(("Images", "*.png *.gif *.bmp *.jpg *.jpeg"),))
			# Check if a image has been selected (aka popup returned a file path)
			if (filename):	
				OriginalImage = PIL.Image.open(filename).convert("RGB")
				window['-RB_COL_24B-'].update(True)
				window['-RB_DIT_FS-'].update(disabled=True)
				window['-RB_DIT_NO-'].update(True)
				window['-TBXPATH-'].update(filename)
				window['-IN_SIZE_X-'].update(OriginalImage.size[0])
				window['-IN_SIZE_Y-'].update(OriginalImage.size[1])
				ImageToDisplay = convertImage(OriginalImage, (OriginalImage.size[0], OriginalImage.size[1]), ConversionType.RGB888, True, ColorIndexLength, ColorIndexTable, False)
				resizeImage(window, ImageToDisplay, ImageOffset)
		# Convert & save the PIF file
		if ((events == 'Save') or (events == '-BTN_SAVE-')):
			if (ImageToDisplay == None): continue
			filename = sg.popup_get_file('Save PIF Image','Save PIF Image', save_as=True, default_extension='Test123', no_window=True, show_hidden=False, file_types=(("PIF Image", "*.pif"),))
			if (filename):
				print(filename)
				conType = ConversionType.UNDEFINED
				if values['-RB_COL_CUS-'] == True:	conType = ColorIndexType
				if values['-RB_COL_1BM-'] == True:	conType = ConversionType.MONOCHROME
				if values['-RB_COL_4C-'] == True:	conType = ConversionType.RGB16C
				if values['-RB_COL_8B-'] == True:	conType = ConversionType.RGB332
				if values['-RB_COL_16B-'] == True:	conType = ConversionType.RGB565
				if values['-RB_COL_24B-'] == True:	conType = ConversionType.RGB888
				imageHeader, colorTable, imageData, rlePos = convertToPIF(OriginalImage, ((int)(values['-IN_SIZE_X-']),(int)(values['-IN_SIZE_Y-'])), conType, ColorIndexLength, ColorIndexTable, values['-RB_DIT_FS-'], values['-RB_COMP_RLE-'])
				isize = savePIFbinary(imageHeader, colorTable, imageData, rlePos, filename)
				compression = values['-RB_COMP_RLE-']
				file_saved(conType.name, compression, isize)

		# Respond to window resize and re-load the image
		if (events == 'WinEvent'):
			if (not ImageToDisplay == None):
				# Strategy: Resize the image only when the resizing of the window has stopped, using a threated.timer in the background
				timer_windowResize.cancel()
				# Since canceling destroys the prepared threading.Timer setup, it has to be recreated - why no Threading.Timer.Reset?
				timer_windowResize = threading.Timer(THREADWAIT, resizeImage,args=(window, ImageToDisplay, ImageOffset))
				# (Re)start the timer to resize the image after THREADWAIT-amount of seconds
				timer_windowResize.start()

		# Disable Dithering for 16bpp or 24bpp option
		if ((events == '-RB_COL_16B-') or (events == '-RB_COL_24B-')):
			window['-RB_DIT_FS-'].update(disabled=True)
			window['-RB_DIT_NO-'].update(True)
		if ((events == '-RB_COL_CUS-') or (events == '-RB_COL_1BM-') or (events == '-RB_COL_4C-') or (events == '-RB_COL_8B-')):
			window['-RB_DIT_FS-'].update(disabled=False)

		# Radio button for Pixel has been (un)checked
		if (events == '-RB_RES_PXL-'):
			print(events, values['-RB_RES_PXL-'])
			if (not ImageToDisplay == None):
				imgXpx = int(ImageToDisplay.size[0])
				imgYpx = int(ImageToDisplay.size[1])	

				window['-IN_SIZE_X-'].update(imgXpx)
				window['-IN_SIZE_Y-'].update(imgYpx)

		# Radio Button for Percentage has been (un)checked
		if (events == '-RB_RES_PRC-'):
			print(events, values['-RB_RES_PRC-'])
			if (not ImageToDisplay == None):
				imgXpx = int(ImageToDisplay.size[0])
				imgYpx = int(ImageToDisplay.size[1])	

				window['-IN_SIZE_X-'].update(100)
				window['-IN_SIZE_Y-'].update(100)
	
		# Check pixel input if valid, and add ratio if required
		if (events == '-IN_SIZE_X-'):
			if (not ImageToDisplay == None):
				try:
					value = (int)(values['-IN_SIZE_X-'])
					if (values['-CB_RAT-'] == True):
						window['-IN_SIZE_Y-'].update(int(value * (OriginalImage.size[1] / OriginalImage.size[0])))
				except:
					window['-IN_SIZE_X-'].update(int(OriginalImage.size[0]))

		# Check pixel input if valid, and add ratio if required
		if (events == '-IN_SIZE_Y-'):
			if (not ImageToDisplay == None):
				try:
					value = (int)(values['-IN_SIZE_Y-'])
					if (values['-CB_RAT-'] == True):
						window['-IN_SIZE_X-'].update(int(value / (OriginalImage.size[1] / OriginalImage.size[0])))
				except:
					window['-IN_SIZE_Y-'].update(int(OriginalImage.size[1]))
	window.close()

sg.theme('Default1')
if __name__ == "__main__":
	main()
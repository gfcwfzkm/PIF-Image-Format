import PySimpleGUI as sg
import PIL.Image
import io
import threading
from enum import Enum

THREADWAIT = 0.1

class ConversionType(Enum):
	UNDEFINED=0
	CUSTOM=1
	MONOCHROME=2
	INDEXED2=3
	INDEXED4=4
	INDEXED8=5
	K256COL=6
	K65COL=7
	UNCHANGED=8

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

def resizeImage():
	oldWindowSize = window.size
	# Resize the image based on the free space available for the [image] element by substracting the offsets from the window size
	# (Basically avoiding to check the [image]-element's size, trying to save calls here by working with stored variables)
	window['-IMGBOX-'].update(convToBytes(ImageToDisplay, (oldWindowSize[0] - ImageOffset[0], oldWindowSize[1] - ImageOffset[1])))

sg.theme('Default1')

menubar_layout = [
	['&File', ['&Open','&Quit']],
	['&Settings', ['&Configure custom option']],
	['&Help', ['&About']]
]

leftCol_layout = [
	[sg.Frame('Colour Settings', [
		[sg.Radio('Custom Configuration', 1, key='-RB_COL_CUS-', enable_events=True)],
		[sg.Radio('1bpp (Monochrome B/W)', 1, key='-RB_COL_1BM-', enable_events=True)],
		[sg.Radio('1bpp (2 Indexed Colours', 1, key='-RB_COL_1BI-', enable_events=True)],
		[sg.Radio('4bpp (16 Indexed Colours', 1, key='-RB_COL_4BI-', enable_events=True)],
		[sg.Radio('8bpp (256 Colours - 3R3G2B)', 1, key='-RB_COL_8B-', enable_events=True)],
		[sg.Radio('16bpp (65586 Colours)', 1, key='-RB_COL_16B-', enable_events=True)],
		[sg.Radio('24bpp (Unchanged image)', 1, key='-RB_COL_24B-', enable_events=True, default=True)]],
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
		[sg.Radio('Pixel', 4, key='-RB_RES_PXL-', enable_events=True, default=True), 
		sg.Radio('Percentage', 4, key='-RB_RES_PRC-', enable_events=True)],
		[sg.Text('W:'), sg.Input(key='-IN_SIZE_X-', size=(8,1)), sg.Text('H:'), sg.Input(key='-IN_SIZE_Y-', size=(8,1))],
		[sg.Checkbox('Lock Ratio', key='-CB_RAT-', enable_events=True, default=True)]],
		expand_x=True)
	],
	[sg.VPush()],
	[sg.Button('Preview', key='-BTN_PREV-', expand_x=True)]
]

rightCol_layout = [
	[sg.Text(' ', key='-TBXPATH-', expand_x=True, justification='center')],
	[sg.Image(key='-IMGBOX-', expand_x=True, expand_y=True)],
	[sg.Text(size=(60,1))]
]

layout = [
	[sg.Menubar(menubar_layout)],
	[sg.Pane([
		sg.Column(leftCol_layout, expand_y=True, expand_x=False, element_justification='l'),
		sg.Column(rightCol_layout, element_justification='c', expand_y=True, expand_x=True)],
		orientation='h', relief=sg.RELIEF_SUNKEN, key='-PANE-')
	]
]

window = sg.Window('Image converter', layout, border_depth=0, resizable=True, finalize=True)
window.bind('<Configure>', "WinEvent")
window.set_min_size((window.size[0], window.size[1]+30))
window['-PANE-'].expand(True, True, True)
window['-IMGBOX-'].expand(True, True)
window.bring_to_front()

oldWindowSize = window.size
# Calculate the space between the window and the image control itself. Will be used to set the image's size
ImageOffset = (window.size[0] - window['-IMGBOX-'].get_size()[0], window.size[1] - window['-IMGBOX-'].get_size()[1])
# Setup the threaded timer beforehand, used in TimerStrategy
timer_windowResize = threading.Timer(THREADWAIT, resizeImage)

OriginalImage = None	# Original Image, do not change
ImageToDisplay = None	# Image to Display and manipulate
ConvertedImage = None	# Result / Converted Image

while True:
	events, values = window.read()

	# Quit Application
	if (events == sg.WIN_CLOSED or events == 'Quit'):
		break

	if (events == '-BTN_PREV-'):
		conType = ConversionType.UNDEFINED
		if values['-RB_COL_CUS-'] == True:	conType = ConversionType.CUSTOM
		if values['-RB_COL_1BM-'] == True:	conType = ConversionType.MONOCHROME
		if values['-RB_COL_1BI-'] == True:	conType = ConversionType.INDEXED2
		if values['-RB_COL_4BI-'] == True:	conType = ConversionType.INDEXED4
		if values['-RB_COL_8B-'] == True:	conType = ConversionType.K256COL
		if values['-RB_COL_16B-'] == True:	conType = ConversionType.K65COL
		if values['-RB_COL_24B-'] == True:	conType = ConversionType.UNCHANGED

		match conType:
			case ConversionType.MONOCHROME:
				ImageToDisplay = OriginalImage.convert("1", dither=values['-RB_DIT_FS-'])
			case ConversionType.INDEXED2:
				ImageToDisplay = OriginalImage.quantize(2, palette=OriginalImage.quantize(2), dither=values['-RB_DIT_FS-'])
			case ConversionType.INDEXED4:
				ImageToDisplay = OriginalImage.quantize(16, palette=OriginalImage.quantize(16), dither=values['-RB_DIT_FS-'])
			case ConversionType.K256COL:
				palImage = PIL.Image.new('P', (16,16))
				palList = []
				for colors in range(256):
					palList.extend([colors & 0b11100000, (colors & 0b00011100) << 3, (colors & 0b00000011) << 6])
				print(len(palList))
				palImage.putpalette(palList)
				ImageToDisplay = OriginalImage.quantize(256, palette=palImage, dither=values['-RB_DIT_FS-'])
			case ConversionType.UNCHANGED:
				ImageToDisplay = OriginalImage

		resizeImage()
		print(conType)

	# Open  image
	if (events == 'Open'):
		filename = sg.popup_get_file('Open Image', no_window=True, show_hidden=False, file_types=(("Images", "*.png *.gif *.bmp *.jpg *.jpeg"),))
		# Check if a image has been selected (aka popup returned a file path)
		if (filename):
			OriginalImage = PIL.Image.open(filename).convert("RGB")
			ImageToDisplay = OriginalImage
			resizeImage()
			window['-RB_COL_24B-'].update(True)
			window['-RB_DIT_FS-'].update(disabled=True)
			window['-RB_DIT_NO-'].update(True)
			window['-TBXPATH-'].update(filename)
			window['-IN_SIZE_X-'].update(ImageToDisplay.size[0])
			window['-IN_SIZE_Y-'].update(ImageToDisplay.size[1])

	# Respond to window resize and re-load the image
	if (events == 'WinEvent'):
		if (not ImageToDisplay == None):
			# Strategy: Resize the image only when the resizing of the window has stopped, using a threated.timer in the background
			timer_windowResize.cancel()
			# Since canceling destroys the prepared threading.Timer setup, it has to be recreated - why no Threading.Timer.Reset?
			timer_windowResize = threading.Timer(THREADWAIT, resizeImage)
			# (Re)start the timer to resize the image after THREADWAIT-amount of seconds
			timer_windowResize.start()

	# Disable Dithering for 16bpp or 24bpp option
	if ((events == '-RB_COL_16B-') or (events == '-RB_COL_24B-')):
		window['-RB_DIT_FS-'].update(disabled=True)
		window['-RB_DIT_NO-'].update(True)
	if ((events == '-RB_COL_CUS-') or (events == '-RB_COL_1BM-') or (events == '-RB_COL_1BI-') or (events == '-RB_COL_4BI-') or (events == '-RB_COL_8B-')):
		window['-RB_DIT_FS-'].update(disabled=False)

	# Radio button for Pixel has been (un)checked
	if (events == '-RB_RES_PXL-'):
		print(events, values['-RB_RES_PXL-'])
		if (not ImageToDisplay == None):
			imgXpx = int(ImageToDisplay.size[0])
			imgYpx = int(ImageToDisplay.size[1])
			print(imgXpx,imgYpx)

			window['-IN_SIZE_X-'].update(imgXpx)
			window['-IN_SIZE_Y-'].update(imgYpx)

			if (events == '-CB_RAT-'):
				print("DoStuff")
	
	# Radio Button for Percentage has been (un)checked
	if (events == '-RB_RES_PRC-'):
		print(events, values['-RB_RES_PRC-'])
		if (not ImageToDisplay == None):
			imgXpx = int(ImageToDisplay.size[0])
			imgYpx = int(ImageToDisplay.size[1])
			print(imgXpx,imgYpx)

			window['-IN_SIZE_X-'].update(100)
			window['-IN_SIZE_Y-'].update(100)

	# Checkbox for Ratio has been (un)checked
	if (events == '-CB_RAT-'):
		print(events, values['-CB_RAT-'])
		if (not ImageToDisplay == None):
			print("DoStuff")


window.close()
print("Application quit")
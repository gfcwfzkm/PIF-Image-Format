from enum import Enum   # Python 3.10 or higher requried

class PIF():
	"""
	PIF Image Decoder and Encoder library
	"""

	COLORTABLE_16C = [
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

	class PIFType(Enum):
		ImageTypeRGB888 = 0x433C
		ImageTypeRGB565 = 0xE5C5
		ImageTypeRGB332 = 0x1E53
		ImageTypeRGB16C = 0xB895
		ImageTypeBLWH   = 0x7DAA
		ImageTypeIND24  = 0x4952
		ImageTypeIND16  = 0x4947
		ImageTypeIND8   = 0x4942
		ImageTypeINDEXED = 0x4900
		ImageTypeUnknown = 0xFFFF
		ColorTableOffset = 28

	class InvalidHeader(Exception):
		"""Exception raised if file header is not valid

    	Attributes:
        	None
    	"""
		def __init__(self, message = "Invalid or unknown PIF Header") -> None:
			self.message = message
			super().__init__(self.message)

	class PIFInfo():
		def __init__(self) -> None:
			self.fileSize = None
			self.imageOffset = None
			self.imageType = PIF.PIFType.ImageTypeUnknown
			self.bitsPetPixel = None
			self.imageWidth = None
			self.imageHeigt = None
			self.imageSize = None
			self.colTableSize = None
			self.compression = PIF.CompressionType.NO_COMPRESSION

	def __init__(self) -> None:
		self.info = self.PIFInfo()
		self.pifByteArray = None
		pass

	def decode(PIFdata: bytearray) -> None | Exception:
		"""Decodes a raw PIF bytearray

		Decodes the PIF image header and image data itself, storing
		it internally in an RGB888 bytearray

		Parameters:
			PIFData : bytearray, mandatory
				Binary PIF data
		
		Returns:
			None or Exception
				Returns nothing if the operation was performed without an error,
				otherwise it will return an Exception message
		
		"""
		return -1
	
	def encode(RGB888: bytearray, imageType: ConversionType, compression: CompressionType) -> None | Exception:
		return -1
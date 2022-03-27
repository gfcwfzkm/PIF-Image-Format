# Currently only a testing ground!!




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
			while ((tempArray[tempIndex - 1] == tempArray[tempIndex - 2]) and (pixelIndex < len(pixelArray) and (tempIndex < 8))):
				tempArray[tempIndex] = pixelArray[pixelIndex]
				tempIndex += 1
				pixelIndex += 1
						
			compressedArray.append(tempIndex - 1)
			compressedArray.append(tempArray[0])

			tempArray[0] = tempArray[tempIndex - 1]
		else:
			# Not compressible
			print('x')

	return compressedArray

def rle_compress(pixelArray):
	outlist = []
	rlePos = []
	tempArray = [None]*129
	outCnt = 99
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
		
			# Check if there is more data to read
			#if not (pixCnt >= len(pixelArray)):
				# More data to read!
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
					outCnt += 1
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

toCompress = [14,15,15,15,16,15]
print(rle_compression(toCompress))
rleposlist, returnlist = rle_compress(toCompress)
print(rleposlist)
print(returnlist)
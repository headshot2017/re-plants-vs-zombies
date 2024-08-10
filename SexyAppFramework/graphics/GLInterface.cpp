#include <SDL.h>
#include <GL/glew.h>
#include <GL/glext.h>

#include "GLInterface.h"
#include "GLImage.h"
#include "SexyAppBase.h"
#include "misc/AutoCrit.h"
#include "misc/CritSect.h"
#include "Graphics.h"
#include "MemoryImage.h"

using namespace Sexy;

static int gMinTextureWidth;
static int gMinTextureHeight;
static int gMaxTextureWidth;
static int gMaxTextureHeight;
static int gSupportedPixelFormats;
static bool gTextureSizeMustBePow2;
static const int MAX_TEXTURE_SIZE = 1024;
static bool gLinearFilter = false;


struct GLVertex
{
	float sx;
	float sy;
	float sz;
	float rhw;
	uint32_t color;
	uint32_t specular;
	float tu;
	float tv;
};


static void CopyImageToTexture8888(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, int theDestPitch, int theDestHeight, bool rightPad, bool bottomPad, bool create)
{
	uint32_t *aDest = new uint32_t[theDestPitch * theDestHeight];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		uint32_t *dstRow = aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint32_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
				*dst++ = *src++;

			if (rightPad)
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	else // palette
	{
		uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uint32_t *dstRow = aDest;
		long unsigned int *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint32_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
				*dst++ = palette[*src++];

			if (rightPad)
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}

	if (bottomPad)
	{
		uint32_t *dstrow = aDest + (theDestPitch*theHeight);
		memcpy(dstrow, dstrow-(theDestPitch*4), (theDestPitch*4));
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, theDestPitch, theDestHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theDestPitch, theDestHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);

	delete[] aDest;
}

static void CopyImageToTexture4444(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, int theDestPitch, int theDestHeight, bool rightPad, bool bottomPad, bool create)
{
	uint16_t *aDest = new uint16_t[theDestPitch * theDestHeight];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		uint16_t *dstRow = aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint16_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = *src++;
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	else // palette
	{
		uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uint16_t *dstRow = aDest;
		long unsigned int *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint16_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = palette[*src++];
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}

	if (bottomPad)
	{
		uint16_t *dstrow = aDest + (theDestPitch*theHeight);
		memcpy(dstrow, dstrow-(theDestPitch*2), (theDestPitch*2));
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, theDestPitch, theDestHeight, 0, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theDestPitch, theDestHeight, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, aDest);

	delete[] aDest;
}

static void CopyImageToTexture565(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, int theDestPitch, int theDestHeight, bool rightPad, bool bottomPad, bool create)
{
	uint16_t *aDest = new uint16_t[theDestPitch * theDestHeight];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		uint16_t *dstRow = aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint16_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = *src++;
				*dst++ = ((aPixel>>8)&0xF800) | ((aPixel>>5)&0x07E0) | ((aPixel>>3)&0x001F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	else
	{
		uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uint16_t *dstRow = aDest;
		long unsigned int *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint16_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = palette[*src++];
				*dst++ = ((aPixel>>8)&0xF800) | ((aPixel>>5)&0x07E0) | ((aPixel>>3)&0x001F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}

	if (bottomPad)
	{
		uint16_t *dstrow = aDest + (theDestPitch*theHeight);
		memcpy(dstrow, dstrow-(theDestPitch*2), (theDestPitch*2));
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, theDestPitch, theDestHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theDestPitch, theDestHeight, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, aDest);

	delete[] aDest;
}

static void CopyImageToTexturePalette8(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, int theDestPitch, int theDestHeight, bool rightPad, bool bottomPad, bool create)
{
	printf("PALETTE %d %d - %d %d - %d %d\n", offx, offy, theWidth, theHeight, theDestPitch, theDestHeight);
	fflush(stdout);

	uint32_t *aDest = new uint32_t[theDestPitch * theDestHeight];

	uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
	uint32_t *dstRow = aDest;
	long unsigned int *palette = theImage->mColorTable;

	for(int y=0; y<theHeight; y++)
	{
		uint8_t *src = srcRow;
		uint32_t *dst = dstRow;
		for(int x=0; x<theWidth; x++)
		{
			long unsigned int aPixel = palette[*src++];
			*dst++ = (aPixel&0xFF00FF00) | ((aPixel>>16)&0xFF) | ((aPixel<<16)&0xFF0000);
		}

		if (rightPad) 
			*dst = *(dst-1);

		srcRow += theImage->GetWidth();
		dstRow += theDestPitch;
	}

	if (bottomPad)
	{
		uint32_t *dstrow = aDest + (theDestPitch*theHeight);
		memcpy(dstrow, dstrow-(theDestPitch*4), (theDestPitch*4));
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, theDestPitch, theDestHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theDestPitch, theDestHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);

	delete[] aDest;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexture(GLuint& theTexture, MemoryImage *theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat, bool create)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (gLinearFilter) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (gLinearFilter) ? GL_LINEAR : GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int aWidth = std::min(texWidth,(theImage->GetWidth()-offx));
	int aHeight = std::min(texHeight,(theImage->GetHeight()-offy));

	bool rightPad = aWidth<texWidth;
	bool bottomPad = aHeight<texHeight;

	if(aWidth>0 && aHeight>0)
	{
		switch (theFormat)
		{
			case PixelFormat_A8R8G8B8:	CopyImageToTexture8888(theImage, offx, offy, aWidth, aHeight, texWidth, texHeight, rightPad, bottomPad, create); break;
			case PixelFormat_A4R4G4B4:	CopyImageToTexture4444(theImage, offx, offy, aWidth, aHeight, texWidth, texHeight, rightPad, bottomPad, create); break;
			case PixelFormat_R5G6B5:	CopyImageToTexture565(theImage, offx, offy, aWidth, aHeight, texWidth, texHeight, rightPad, bottomPad, create); break;
			case PixelFormat_Palette8:	CopyImageToTexturePalette8(theImage, offx, offy, aWidth, aHeight, texWidth, texHeight, rightPad, bottomPad, create); break;
			case PixelFormat_Unknown: break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int GetClosestPowerOf2Above(int theNum)
{
	int aPower2 = 1;
	while (aPower2 < theNum)
		aPower2<<=1;
	return aPower2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool IsPowerOf2(int theNum)
{
	int aNumBits = 0;
	while (theNum>0)
	{
		aNumBits += theNum&1;
		theNum >>= 1;
	}

	return aNumBits==1;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void GetBestTextureDimensions(int &theWidth, int &theHeight, bool isEdge, bool usePow2, long unsigned int theImageFlags)
{
//	theImageFlags = D3DImageFlag_MinimizeNumSubdivisions;
	if (theImageFlags & D3DImageFlag_Use64By64Subdivisions)
	{
		theWidth = theHeight = 64;
		return;
	}

	static int aGoodTextureSize[MAX_TEXTURE_SIZE];
	static bool haveInited = false;
	if (!haveInited)
	{
		haveInited = true;
		int i;
		int aPow2 = 1;
		for (i=0; i<MAX_TEXTURE_SIZE; i++)
		{
			if (i > aPow2)
				aPow2 <<= 1;

			int aGoodValue = aPow2;
			if ((aGoodValue - i ) > 64)
			{
				aGoodValue >>= 1;
				while (true)
				{
					int aLeftOver = i % aGoodValue;
					if (aLeftOver<64 || IsPowerOf2(aLeftOver))
						break;

					aGoodValue >>= 1;
				}
			}
			aGoodTextureSize[i] = aGoodValue;
		}
	}

	int aWidth = theWidth;
	int aHeight = theHeight;

	if (usePow2)
	{
		if (isEdge || (theImageFlags & D3DImageFlag_MinimizeNumSubdivisions))
		{
			aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : GetClosestPowerOf2Above(aWidth);
			aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : GetClosestPowerOf2Above(aHeight);
		}
		else
		{
			aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : aGoodTextureSize[aWidth];
			aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : aGoodTextureSize[aHeight];
		}
	}

	if (aWidth < gMinTextureWidth)
		aWidth = gMinTextureWidth;

	if (aHeight < gMinTextureHeight)
		aHeight = gMinTextureHeight;

	theWidth = aWidth;
	theHeight = aHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TextureData::TextureData()
{
	mWidth = 0;
	mHeight = 0;
	mTexVecWidth = 0;
	mTexVecHeight = 0;
	mBitsChangedCount = 0;
	mTexMemSize = 0;
	mTexPieceWidth = 64;
	mTexPieceHeight = 64;

	//mPalette = NULL;
	mPixelFormat = PixelFormat_Unknown;
	mImageFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TextureData::~TextureData()
{
	ReleaseTextures();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::ReleaseTextures()
{
	for(int i=0; i<(int)mTextures.size(); i++)
	{
		GLuint* aSurface = &mTextures[i].mTexture;
		glDeleteTextures(1, aSurface);
	}

	mTextures.clear();

	mTexMemSize = 0;

	/*
	if (mPalette!=NULL)
	{
		mPalette->Release();
		mPalette = NULL;
	}
	*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CreateTextureDimensions(MemoryImage *theImage)
{
	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();
	unsigned int i;

	// Calculate inner piece sizes
	mTexPieceWidth = aWidth;
	mTexPieceHeight = aHeight;
	bool usePow2 = true; //gTextureSizeMustBePow2 || mPixelFormat==PixelFormat_Palette8;
	GetBestTextureDimensions(mTexPieceWidth, mTexPieceHeight, false, usePow2, mImageFlags);

	// Calculate right boundary piece sizes
	int aRightWidth = aWidth % mTexPieceWidth;
	int aRightHeight = mTexPieceHeight;
	if (aRightWidth > 0)
		GetBestTextureDimensions(aRightWidth, aRightHeight, true, usePow2, mImageFlags);
	else
		aRightWidth = mTexPieceWidth;

	// Calculate bottom boundary piece sizes
	int aBottomWidth = mTexPieceWidth;
	int aBottomHeight = aHeight % mTexPieceHeight;
	if (aBottomHeight > 0)
		GetBestTextureDimensions(aBottomWidth, aBottomHeight, true, usePow2, mImageFlags);
	else
		aBottomHeight = mTexPieceHeight;

	// Calculate corner piece size
	int aCornerWidth = aRightWidth;
	int aCornerHeight = aBottomHeight;
	GetBestTextureDimensions(aCornerWidth, aCornerHeight, true, usePow2, mImageFlags);

	// Allocate texture array
	mTexVecWidth = (aWidth + mTexPieceWidth - 1) / mTexPieceWidth;
	mTexVecHeight = (aHeight + mTexPieceHeight - 1) / mTexPieceHeight;
	mTextures.resize(mTexVecWidth * mTexVecHeight);

	// Assign inner pieces
	for (i = 0; i < mTextures.size(); i++)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mTexture = 0;
		aPiece.mWidth = mTexPieceWidth;
		aPiece.mHeight = mTexPieceHeight;
	}

	// Assign right pieces
	for (i = mTexVecWidth - 1; i < mTextures.size(); i += mTexVecWidth)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mWidth = aRightWidth;
		aPiece.mHeight = aRightHeight;
	}

	// Assign bottom pieces
	for (i = mTexVecWidth * (mTexVecHeight - 1); i < mTextures.size(); i++)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mWidth = aBottomWidth;
		aPiece.mHeight = aBottomHeight;
	}

	// Assign corner piece
	mTextures.back().mWidth = aCornerWidth;
	mTextures.back().mHeight = aCornerHeight;
	/**/

	mMaxTotalU = aWidth / (float)mTexPieceWidth;
	mMaxTotalV = aHeight / (float)mTexPieceHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CreateTextures(MemoryImage *theImage)
{
	theImage->DeleteSWBuffers(); // don't need these buffers for 3d drawing

	// Choose appropriate pixel format
	PixelFormat aFormat = PixelFormat_A8R8G8B8;
	//theImage->mD3DFlags = D3DImageFlag_UseA4R4G4B4;

	theImage->CommitBits();
	if (!theImage->mHasAlpha && !theImage->mHasTrans && (gSupportedPixelFormats & PixelFormat_R5G6B5))
	{
		if (!(theImage->mD3DFlags & D3DImageFlag_UseA8R8G8B8))
			aFormat = PixelFormat_R5G6B5;
	}

	if (theImage->mColorIndices != NULL && (gSupportedPixelFormats & PixelFormat_Palette8))
	{
		/*
		palEntry aPalette[256];
		for (int i=0; i<256; i++)
		{
			uint32_t aPixel = theImage->mColorTable[i];
			*(uint32_t*)(aPalette+i) = (aPixel&0xFF00FF00) | ((aPixel>>16)&0xFF) | ((aPixel<<16)&0xFF0000);
		}
		HRESULT aResult = theDraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALPHA | DDPCAPS_ALLOW256,aPalette, &aDDPalette, NULL);
		if (SUCCEEDED(aResult))
			aFormat = PixelFormat_Palette8;
		else
		{
			std::string anError = GetDirectXErrorString(aResult);
			gSupportedPixelFormats &= ~PixelFormat_Palette8;
		}
		*/
	}

	if ((theImage->mD3DFlags & D3DImageFlag_UseA4R4G4B4) && aFormat==PixelFormat_A8R8G8B8 && (gSupportedPixelFormats & PixelFormat_A4R4G4B4))
		aFormat = PixelFormat_A4R4G4B4;

	if (aFormat==PixelFormat_A8R8G8B8 && !(gSupportedPixelFormats & PixelFormat_A8R8G8B8))
		aFormat = PixelFormat_A4R4G4B4;

	// Release texture if image size has changed
	bool createTextures = false;
	if (mWidth!=theImage->mWidth || mHeight!=theImage->mHeight || aFormat!=mPixelFormat || theImage->mD3DFlags!=mImageFlags)
	{
		ReleaseTextures();

		mPixelFormat = aFormat;
		mImageFlags = theImage->mD3DFlags;
		CreateTextureDimensions(theImage);
		createTextures = true;
	}

	int i,x,y;

	int aHeight = theImage->GetHeight();
	int aWidth = theImage->GetWidth();

	int aFormatSize = 4;
	if (aFormat==PixelFormat_R5G6B5)
		aFormatSize = 2;
	else if (aFormat==PixelFormat_A4R4G4B4)
		aFormatSize = 2;

	i=0;
	for(y=0; y<aHeight; y+=mTexPieceHeight)
	{
		for(x=0; x<aWidth; x+=mTexPieceWidth, i++)
		{
			TextureDataPiece &aPiece = mTextures[i];
			if (createTextures)
			{
				glGenTextures(1, &aPiece.mTexture);
				mTexMemSize += aPiece.mWidth*aPiece.mHeight*aFormatSize;
			}
			glBindTexture(GL_TEXTURE_2D, aPiece.mTexture);

			CopyImageToTexture(aPiece.mTexture,theImage,x,y,aPiece.mWidth,aPiece.mHeight,aFormat, createTextures);
		}
	}

	mWidth = theImage->mWidth;
	mHeight = theImage->mHeight;
	mBitsChangedCount = theImage->mBitsChangedCount;
	mPixelFormat = aFormat;
}
	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CheckCreateTextures(MemoryImage *theImage)
{
	if(mPixelFormat==PixelFormat_Unknown || theImage->mWidth != mWidth || theImage->mHeight != mHeight || theImage->mBitsChangedCount != mBitsChangedCount || theImage->mD3DFlags != mImageFlags)
		CreateTextures(theImage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GLuint& TextureData::GetTexture(int x, int y, int &width, int &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	int left = x%mTexPieceWidth;
	int top = y%mTexPieceHeight;
	int right = left+width;
	int bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GLuint& TextureData::GetTextureF(float x, float y, float &width, float &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	float left = x - tx*mTexPieceWidth;
	float top = y - ty*mTexPieceHeight;
	float right = left+width;
	float bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void SetLinearFilter(bool linear)
{
	if (gLinearFilter != linear)
	{
		int aFilter = (linear) ? GL_LINEAR : GL_NEAREST;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);

		gLinearFilter = linear;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::Blt(float theX, float theY, const Rect& theSrcRect, const Color& theColor)
{
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth,aHeight;
	float u1,v1,u2,v2;

	srcY = srcTop;
	dstY = theY;

	long unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	glEnable(GL_TEXTURE_2D);

	while(srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = theX;
		while(srcX < srcRight)
		{
			aWidth = srcRight-srcX;
			aHeight = srcBottom-srcY;
			GLuint& aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			float x = dstX - 0.5f;
			float y = dstY - 0.5f;

			GLVertex aVertex[4] = {
				{ {x},        {y},         {0},{1},{aColor},{0},{u1},{v1} },
				{ {x},        {y+aHeight}, {0},{1},{aColor},{0},{u1},{v2} },
				{ {x+aWidth}, {y},         {0},{1},{aColor},{0},{u2},{v1} },
				{ {x+aWidth}, {y+aHeight}, {0},{1},{aColor},{0},{u2},{v2} }
			};

			glBindTexture(GL_TEXTURE_2D, aTexture);

			glBegin(GL_TRIANGLE_STRIP);
			for (int i=0; i<4; i++)
			{
				glColor4ubv((uint8_t*)&aVertex[i].color);
				glTexCoord2f(aVertex[i].tu, aVertex[i].tv);
				glVertex2f(aVertex[i].sx, aVertex[i].sy);
			}
			glEnd();

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}
}

struct VertexList
{
	enum { MAX_STACK_VERTS = 100 };
	GLVertex mStackVerts[MAX_STACK_VERTS];
	GLVertex* mVerts;
	int mSize;
	int mCapacity;

	typedef int size_type;

	VertexList() : mVerts(mStackVerts), mSize(0), mCapacity(MAX_STACK_VERTS) { }
	VertexList(const VertexList& theList) : mVerts(mStackVerts), mSize(theList.mSize), mCapacity(MAX_STACK_VERTS)
	{
		reserve(mSize);
		memcpy(mVerts, theList.mVerts, mSize * sizeof(mVerts[0]));
	}

	~VertexList()
	{
		if (mVerts != mStackVerts)
			delete mVerts;
	}

	void reserve(int theCapacity)
	{
		if (mCapacity < theCapacity)
		{
			mCapacity = theCapacity;
			GLVertex* aNewList = new GLVertex[theCapacity];
			memcpy(aNewList, mVerts, mSize * sizeof(mVerts[0]));
			if (mVerts != mStackVerts)
				delete mVerts;

			mVerts = aNewList;
		}
	}

	void push_back(const GLVertex& theVert)
	{
		if (mSize == mCapacity)
			reserve(mCapacity * 2);

		mVerts[mSize++] = theVert;
	}

	void operator=(const VertexList& theList)
	{
		reserve(theList.mSize);
		mSize = theList.mSize;
		memcpy(mVerts, theList.mVerts, mSize * sizeof(mVerts[0]));
	}


	GLVertex& operator[](int thePos)
	{
		return mVerts[thePos];
	}

	int size() { return mSize; }
	void clear() { mSize = 0; }
};

static inline float GetCoord(const GLVertex& theVertex, int theCoord)
{
	switch (theCoord)
	{
	case 0: return theVertex.sx;
	case 1: return theVertex.sy;
	case 2: return theVertex.sz;
	case 3: return theVertex.tu;
	case 4: return theVertex.tv;
	default: return 0;
	}
}

static inline GLVertex Interpolate(const GLVertex &v1, const GLVertex &v2, float t)
{
	GLVertex aVertex = v1;
	aVertex.sx = v1.sx + t*(v2.sx-v1.sx);
	aVertex.sy = v1.sy + t*(v2.sy-v1.sy);
	aVertex.tu = v1.tu + t*(v2.tu-v1.tu);
	aVertex.tv = v1.tv + t*(v2.tv-v1.tv);
	if (v1.color!=v2.color)
	{
		int r = ((v1.color >> 0) & 0xff) + t*( ((v2.color >> 0) & 0xff) - ((v1.color >> 0) & 0xff) );
		int g = ((v1.color >> 8) & 0xff) + t*( ((v2.color >> 8) & 0xff) - ((v1.color >> 8) & 0xff) );
		int b = ((v1.color >> 16) & 0xff) + t*( ((v2.color >> 16) & 0xff) - ((v1.color >> 16) & 0xff) );
		int a = ((v1.color >> 24) & 0xff) + t*( ((v2.color >> 24) & 0xff) - ((v1.color >> 24) & 0xff) );
		aVertex.color = (r << 0) | (g << 8) | (b << 16) | (a << 24);
	}

	return aVertex;
}

template<class Pred>
struct PointClipper
{
	Pred mPred;

	void ClipPoint(int n, float clipVal, const GLVertex& v1, const GLVertex& v2, VertexList& out)
	{
		if (!mPred(GetCoord(v1, n), clipVal))
		{
			if (!mPred(GetCoord(v2, n), clipVal)) // both inside
				out.push_back(v2);
			else // inside -> outside
			{
				float t = (clipVal - GetCoord(v1, n)) / (GetCoord(v2, n) - GetCoord(v1, n));
				out.push_back(Interpolate(v1, v2, t));
			}
		}
		else
		{
			if (!mPred(GetCoord(v2, n), clipVal)) // outside -> inside
			{
				float t = (clipVal - GetCoord(v1, n)) / (GetCoord(v2, n) - GetCoord(v1, n));
				out.push_back(Interpolate(v1, v2, t));
				out.push_back(v2);
			}
			//			else // outside -> outside
		}
	}

	void ClipPoints(int n, float clipVal, VertexList& in, VertexList& out)
	{
		if (in.size() < 2)
			return;

		ClipPoint(n, clipVal, in[in.size() - 1], in[0], out);
		for (VertexList::size_type i = 0; i < in.size() - 1; i++)
			ClipPoint(n, clipVal, in[i], in[i + 1], out);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void DrawPolyClipped(const Rect *theClipRect, const VertexList &theList)
{
	VertexList l1, l2;
	l1 = theList;

	int left = theClipRect->mX;
	int right = left + theClipRect->mWidth;
	int top = theClipRect->mY;
	int bottom = top + theClipRect->mHeight;

	VertexList *in = &l1, *out = &l2;
	PointClipper<std::less<float> > aLessClipper;
	PointClipper<std::greater_equal<float> > aGreaterClipper;

	aLessClipper.ClipPoints(0,left,*in,*out); std::swap(in,out); out->clear();
	aLessClipper.ClipPoints(1,top,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(0,right,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(1,bottom,*in,*out);

	VertexList &aList = *out;

	if (aList.size() >= 3)
	{
		glBegin(GL_TRIANGLE_FAN);
		for (unsigned i=0; i<aList.size(); i++)
		{
			GLVertex& v = aList[i];

			glColor4ubv((uint8_t*)&v.color);
			glTexCoord2f(v.tu, v.tv);
			glVertex2f(v.sx, v.sy);
		}
		glEnd();
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void DoPolyTextureClip(VertexList &theList)
{
	VertexList l2;

	float left = 0;
	float right = 1;
	float top = 0;
	float bottom = 1;

	VertexList *in = &theList, *out = &l2;
	PointClipper<std::less<float> > aLessClipper;
	PointClipper<std::greater_equal<float> > aGreaterClipper;

	aLessClipper.ClipPoints(3,left,*in,*out); std::swap(in,out); out->clear();
	aLessClipper.ClipPoints(4,top,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(3,right,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(4,bottom,*in,*out);
}


void TextureData::BltTransformed(const SexyMatrix3 &theTrans, const Rect& theSrcRect, const Color& theColor, const Rect *theClipRect, float theX, float theY, bool center)
{
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth;
	int aHeight;
	float u1,v1,u2,v2;
	float startx = 0, starty = 0;
	float pixelcorrect = 0.5f;

	if (center)
	{
		startx = -theSrcRect.mWidth/2.0f;
		starty = -theSrcRect.mHeight/2.0f;
		pixelcorrect = 0.0f;
	}			

	srcY = srcTop;
	dstY = starty;

	long unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	glEnable(GL_TEXTURE_2D);

	while(srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = startx;
		while(srcX < srcRight)
		{
			aWidth = srcRight-srcX;
			aHeight = srcBottom-srcY;
			GLuint& aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			float x = dstX; // - 0.5f;
			float y = dstY; // - 0.5f;

			SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y+aHeight), SexyVector2(x+aWidth, y) , SexyVector2(x+aWidth, y+aHeight) };
			SexyVector2 tp[4];

			int i;
			for (i=0; i<4; i++)
			{
				tp[i] = theTrans*p[i];
				tp[i].x -= pixelcorrect - theX;
				tp[i].y -= pixelcorrect - theY;
			}

			bool clipped = false;
			if (theClipRect != NULL)
			{
				int left = theClipRect->mX;
				int right = left + theClipRect->mWidth;
				int top = theClipRect->mY;
				int bottom = top + theClipRect->mHeight;
				for (i=0; i<4; i++)
				{
					if (tp[i].x<left || tp[i].x>=right || tp[i].y<top || tp[i].y>=bottom)
					{
						clipped = true;
						break;
					}
				}
			}

			GLVertex aVertex[4] = {
				{ {tp[0].x},{tp[0].y},{0},{1},{aColor},{0},{u1},{v1} },
				{ {tp[1].x},{tp[1].y},{0},{1},{aColor},{0},{u1},{v2} },
				{ {tp[2].x},{tp[2].y},{0},{1},{aColor},{0},{u2},{v1} },
				{ {tp[3].x},{tp[3].y},{0},{1},{aColor},{0},{u2},{v2} }
			};

			glBindTexture(GL_TEXTURE_2D, aTexture);

			if (!clipped)
			{
				glBegin(GL_TRIANGLE_STRIP);
				for (int i=0; i<4; i++)
				{
					glColor4ubv((uint8_t*)&aVertex[i].color);
					glTexCoord2f(aVertex[i].tu, aVertex[i].tv);
					glVertex2f(aVertex[i].sx, aVertex[i].sy);
				}
				glEnd();
			}
			else
			{
				VertexList aList;
				aList.push_back(aVertex[0]);
				aList.push_back(aVertex[1]);
				aList.push_back(aVertex[3]);
				aList.push_back(aVertex[2]);

				DrawPolyClipped(theClipRect, aList);
			}

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}
}

#define GetColorFromTriVertex(theVertex, theColor) (theVertex.color?theVertex.color:theColor)

void TextureData::BltTriangles(const TriVertex theVertices[][3], int theNumTriangles, unsigned int theColor, float tx, float ty)
{
	if ((mMaxTotalU <= 1.0) && (mMaxTotalV <= 1.0))
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mTextures[0].mTexture);

		glBegin(GL_TRIANGLES);

		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			TriVertex* aTriVerts = (TriVertex*) theVertices[aTriangleNum];

			for (int i = 0; i < 3; i++)
			{
				unsigned int aColor = GetColorFromTriVertex(aTriVerts[i], theColor);

				glColor4ubv((uint8_t*)&aColor);
				glTexCoord2f(aTriVerts[i].u * mMaxTotalU, aTriVerts[i].v * mMaxTotalV);
				glVertex2f(aTriVerts[i].x + tx, aTriVerts[i].y + ty);
			}
		}

		glEnd();
	}
	else
	{
		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			TriVertex* aTriVerts = (TriVertex*) theVertices[aTriangleNum];

			GLVertex aVertex[3] = {
				{ {aTriVerts[0].x + tx},{aTriVerts[0].y + ty},	{0},{1},{GetColorFromTriVertex(aTriVerts[0],theColor)},	{0},{aTriVerts[0].u*mMaxTotalU},{aTriVerts[0].v*mMaxTotalV} },
				{ {aTriVerts[1].x + tx},{aTriVerts[1].y + ty},	{0},{1},{GetColorFromTriVertex(aTriVerts[1],theColor)},	{0},{aTriVerts[1].u*mMaxTotalU},{aTriVerts[1].v*mMaxTotalV} },
				{ {aTriVerts[2].x + tx},{aTriVerts[2].y + ty},	{0},{1},{GetColorFromTriVertex(aTriVerts[2],theColor)},	{0},{aTriVerts[2].u*mMaxTotalU},{aTriVerts[2].v*mMaxTotalV} }
			};

			float aMinU = mMaxTotalU, aMinV = mMaxTotalV;
			float aMaxU = 0, aMaxV = 0;

			int i,j,k;
			for (i=0; i<3; i++)
			{
				if(aVertex[i].tu < aMinU)
					aMinU = aVertex[i].tu;

				if(aVertex[i].tv < aMinV)
					aMinV = aVertex[i].tv;

				if(aVertex[i].tu > aMaxU)
					aMaxU = aVertex[i].tu;

				if(aVertex[i].tv > aMaxV)
					aMaxV = aVertex[i].tv;
			}

			VertexList aMasterList;
			aMasterList.push_back(aVertex[0]);
			aMasterList.push_back(aVertex[1]);
			aMasterList.push_back(aVertex[2]);


			VertexList aList;

			int aLeft = floorf(aMinU);
			int aTop = floorf(aMinV);
			int aRight = ceilf(aMaxU);
			int aBottom = ceilf(aMaxV);
			if (aLeft < 0)
				aLeft = 0;
			if (aTop < 0)
				aTop = 0;
			if (aRight > mTexVecWidth)
				aRight = mTexVecWidth;
			if (aBottom > mTexVecHeight)
				aBottom = mTexVecHeight;

			TextureDataPiece &aStandardPiece = mTextures[0];
			for (i=aTop; i<aBottom; i++)
			{
				for (j=aLeft; j<aRight; j++)
				{
					TextureDataPiece &aPiece = mTextures[i*mTexVecWidth + j];


					VertexList aList = aMasterList;
					for(k=0; k<3; k++)
					{
						aList[k].tu -= j;
						aList[k].tv -= i;
						if (i==mTexVecHeight-1)
							aList[k].tv *= (float)aStandardPiece.mHeight / aPiece.mHeight;
						if (j==mTexVecWidth-1)
							aList[k].tu *= (float)aStandardPiece.mWidth / aPiece.mWidth;
					}

					DoPolyTextureClip(aList);
					if (aList.size() >= 3)
					{
						glBindTexture(GL_TEXTURE_2D, aPiece.mTexture);
						glBegin(GL_TRIANGLE_FAN);
						for (unsigned i=0; i<aList.size(); i++)
						{
							GLVertex& v = aList[i];

							glColor4ubv((uint8_t*)&v.color);
							glTexCoord2f(v.tu, v.tv);
							glVertex2f(v.sx, v.sy);
						}
						glEnd();
					}
				}
			}
		}
	}
}


GLInterface::GLInterface(SexyAppBase* theApp)
{
	mApp = theApp;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;

	mPresentationRect = Rect( 0, 0, mWidth, mHeight );

	SDL_DisplayMode aMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(mApp->mSDLWindow), &aMode);
	mRefreshRate = aMode.refresh_rate;
	if (!mRefreshRate) mRefreshRate = 60;
	mMillisecondsPerFrame = 1000/mRefreshRate;

	mScreenImage = 0;

	mNextCursorX = 0;
	mNextCursorY = 0;
	mCursorX = 0;
	mCursorY = 0;
}

GLInterface::~GLInterface()
{
	Flush();

	ImageSet::iterator anItr;
	for(anItr = mImageSet.begin(); anItr != mImageSet.end(); ++anItr)
	{
		MemoryImage *anImage = *anItr;
		delete (TextureData*)anImage->mD3DData;
		anImage->mD3DData = NULL;
	}
}

void GLInterface::SetDrawMode(int theDrawMode)
{
	if (theDrawMode == Graphics::DRAWMODE_NORMAL)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else // Additive
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
}

void GLInterface::AddGLImage(GLImage* theGLImage)
{
	AutoCrit anAutoCrit(mCritSect);

	mGLImageSet.insert(theGLImage);
}

void GLInterface::RemoveGLImage(GLImage* theGLImage)
{
	AutoCrit anAutoCrit(mCritSect);

	GLImageSet::iterator anItr = mGLImageSet.find(theGLImage);
	if (anItr != mGLImageSet.end())
		mGLImageSet.erase(anItr);
}

void GLInterface::Remove3DData(MemoryImage* theImage)
{
	if (theImage->mD3DData != NULL)
	{
		delete (TextureData*)theImage->mD3DData;
		theImage->mD3DData = NULL;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.erase(theImage);
	}
}

GLImage* GLInterface::GetScreenImage()
{
	return mScreenImage;
}

static void GLAPIENTRY MessageCallback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
		type, severity, message );
	fflush(stderr);
}

void GLInterface::UpdateViewport()
{
	// Restrict to 4:3
	// https://bumbershootsoft.wordpress.com/2018/11/29/forcing-an-aspect-ratio-in-3d-with-opengl/

	int width, viewport_width;
	int height, viewport_height;
	int viewport_x = 0;
	int viewport_y = 0;

	SDL_GL_GetDrawableSize(mApp->mSDLWindow, &width, &height);

	glClear(GL_COLOR_BUFFER_BIT);
	Flush();

	viewport_width = width;
	viewport_height = height;
	if (width * 3 > height * 4)
	{
		viewport_width = height * 4 / 3;
		viewport_x = (width - viewport_width) / 2;
	}
	else if (width * 3 < height * 4)
	{
		viewport_height = width * 3 / 4;
		viewport_y = (height - viewport_height) / 2;
	}
	glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
	mPresentationRect = Rect( viewport_x, viewport_y, viewport_width, viewport_height );

	glClear(GL_COLOR_BUFFER_BIT);
	Flush();
}

int GLInterface::Init(bool IsWindowed)
{
	int aMaxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &aMaxSize);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	gTextureSizeMustBePow2 = false;
	gMinTextureWidth = 8;
	gMinTextureHeight = 8;
	gMaxTextureWidth = aMaxSize;
	gMaxTextureHeight = aMaxSize;
	gSupportedPixelFormats = PixelFormat_A8R8G8B8 | PixelFormat_A4R4G4B4 | PixelFormat_R5G6B5 | PixelFormat_Palette8;
	gLinearFilter = false;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, mWidth-1, mHeight-1, 0, -10, 10);

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_DITHER);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	mRGBBits = 32;

	mRedBits = 8;
	mGreenBits = 8;
	mBlueBits = 8;
	
	mRedShift = 0;
	mGreenShift = 8;
	mBlueShift = 16;

	mRedMask = (0xFFU << mRedShift);
	mGreenMask = (0xFFU << mGreenShift);
	mBlueMask = (0xFFU << mBlueShift);

	SetVideoOnlyDraw(false);

	return 1;
}

bool GLInterface::Redraw(Rect* theClipRect)
{
	Flush();
	return true;
}

void GLInterface::SetVideoOnlyDraw(bool videoOnly)
{
	if (mScreenImage) delete mScreenImage;
	mScreenImage = new GLImage(this);
	//mScreenImage->SetSurface(useSecondary ? mSecondarySurface : mDrawSurface);		
	//mScreenImage->mNoLock = mVideoOnlyDraw;
	//mScreenImage->mVideoMemory = mVideoOnlyDraw;
	mScreenImage->mWidth = mWidth;
	mScreenImage->mHeight = mHeight;
	mScreenImage->SetImageMode(false, false);
}

void GLInterface::SetCursorPos(int theCursorX, int theCursorY)
{
	mNextCursorX = theCursorX;
	mNextCursorY = theCursorY;
}

bool GLInterface::PreDraw()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return true;
}

void GLInterface::Flush()
{
	SDL_GL_SwapWindow(mApp->mSDLWindow);
}

bool GLInterface::CreateImageTexture(MemoryImage *theImage)
{
	bool wantPurge = false;

	if(theImage->mD3DData==NULL)
	{
		theImage->mD3DData = new TextureData();
		
		// The actual purging was deferred
		wantPurge = theImage->mPurgeBits;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.insert(theImage);
	}

	TextureData *aData = (TextureData*)theImage->mD3DData;
	aData->CheckCreateTextures(theImage);
	
	if (wantPurge)
		theImage->PurgeBits();

	return aData->mPixelFormat != PixelFormat_Unknown;
}

bool GLInterface::RecoverBits(MemoryImage* theImage)
{
	if (theImage->mD3DData == NULL)
		return false;

	TextureData* aData = (TextureData*) theImage->mD3DData;
	if (aData->mBitsChangedCount != theImage->mBitsChangedCount) // bits have changed since texture was created
		return false;

	printf("recover\n");
	fflush(stdout);
	for (int aPieceRow = 0; aPieceRow < aData->mTexVecHeight; aPieceRow++)
	{
		for (int aPieceCol = 0; aPieceCol < aData->mTexVecWidth; aPieceCol++)
		{
			TextureDataPiece* aPiece = &aData->mTextures[aPieceRow*aData->mTexVecWidth + aPieceCol];

			int offx = aPieceCol*aData->mTexPieceWidth;
			int offy = aPieceRow*aData->mTexPieceHeight;
			int aWidth = std::min(theImage->mWidth-offx, aPiece->mWidth);
			int aHeight = std::min(theImage->mHeight-offy, aPiece->mHeight);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, aPiece->mTexture);

			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, theImage->GetBits());

			/*
			switch (aData->mPixelFormat)
			{
			case PixelFormat_A8R8G8B8:	CopyTexture8888ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_A4R4G4B4:	CopyTexture4444ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_R5G6B5: CopyTexture565ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_Palette8:	CopyTexturePalette8ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight, aData->mPalette); break;
			case PixelFormat_Unknown: break;
			}

			D3DInterface::CheckDXError(aPiece->mTexture->Unlock(NULL),"Texture Unlock");
			*/
		}
	}

	return true;
}

void GLInterface::PushTransform(const SexyMatrix3 &theTransform, bool concatenate)
{
	if (mTransformStack.empty() || !concatenate)
		mTransformStack.push_back(theTransform);
	else
	{
		SexyMatrix3 &aTrans = mTransformStack.back();
		mTransformStack.push_back(theTransform*aTrans);
	}
}

void GLInterface::PopTransform()
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

void GLInterface::Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	if (!mTransformStack.empty())
	{
		BltClipF(theImage,theX,theY,theSrcRect,NULL,theColor,theDrawMode);
		return;
	}

	if (!PreDraw())
		return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*) theImage;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	SetLinearFilter(linearFilter);
	aData->Blt(theX,theY,theSrcRect,theColor);
}

void GLInterface::BltClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect *theClipRect, const Color& theColor, int theDrawMode)
{
	SexyTransform2D aTransform;
	aTransform.Translate(theX, theY);

	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,true);
}

void GLInterface::BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	SexyTransform2D aTransform;		

	aTransform.Translate(-theSrcRect.mWidth,0);
	aTransform.Scale(-1, 1);
	aTransform.Translate(theX, theY);

	BltTransformed(theImage,NULL,theColor,theDrawMode,theSrcRect,aTransform,linearFilter);
}

void GLInterface::StretchBlt(Image* theImage,  const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color &theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	float xScale = (float)theDestRect.mWidth / theSrcRect.mWidth;
	float yScale = (float)theDestRect.mHeight / theSrcRect.mHeight;

	SexyTransform2D aTransform;
	if (mirror)
	{
		aTransform.Translate(-theSrcRect.mWidth,0);
		aTransform.Scale(-xScale, yScale);
	}
	else
		aTransform.Scale(xScale, yScale);

	aTransform.Translate(theDestRect.mX, theDestRect.mY);
	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,!fastStretch);
}

void GLInterface::BltRotated(Image* theImage, float theX, float theY, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY, const Rect& theSrcRect)
{
	SexyTransform2D aTransform;

	aTransform.Translate(-theRotCenterX, -theRotCenterY);
	aTransform.RotateRad(theRot);
	aTransform.Translate(theX+theRotCenterX,theY+theRotCenterY);

	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,true);
}

void GLInterface::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, const SexyMatrix3 &theTransform, bool linearFilter, float theX, float theY, bool center)
{
	if (!PreDraw())
		return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*) theImage;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	if (!mTransformStack.empty())
	{
		SetLinearFilter(true); // force linear filtering in the case of a global transform
		if (theX!=0 || theY!=0)
		{
			SexyTransform2D aTransform;
			if (center)
				aTransform.Translate(-theSrcRect.mWidth/2.0f,-theSrcRect.mHeight/2.0f);

			aTransform = theTransform * aTransform;
			aTransform.Translate(theX,theY);
			aTransform = mTransformStack.back() * aTransform;

			aData->BltTransformed(aTransform, theSrcRect, theColor, theClipRect);
		}
		else
		{
			SexyTransform2D aTransform = mTransformStack.back()*theTransform;
			aData->BltTransformed(aTransform, theSrcRect, theColor, theClipRect, theX, theY, center);
		}
	}
	else
	{
		SetLinearFilter(linearFilter);
		aData->BltTransformed(theTransform, theSrcRect, theColor, theClipRect, theX, theY, center);
	}
}

void GLInterface::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	float x1, y1, x2, y2;

	if (!mTransformStack.empty())
	{
		SexyVector2 p1(theStartX,theStartY);
		SexyVector2 p2(theEndX,theEndY);
		p1 = mTransformStack.back()*p1;
		p2 = mTransformStack.back()*p2;

		x1 = p1.x;
		y1 = p1.y;
		x2 = p2.x;
		y2 = p2.y;
	}
	else
	{
		x1 = theStartX;
		y1 = theStartY;
		x2 = theEndX;
		y2 = theEndY;
	}

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINE_STRIP);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x1, y1);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x2, y2);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x2+0.5f, y2+0.5f);
	glEnd();
}

void GLInterface::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	float x = theRect.mX - 0.5f;
	float y = theRect.mY - 0.5f;
	float aWidth = theRect.mWidth;
	float aHeight = theRect.mHeight;

	float aVertex[4][2] = {
		{x, y},
		{x, y+aHeight},
		{x+aWidth, y},
		{x+aWidth, y+aHeight},
	};

	if (!mTransformStack.empty())
	{
		SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y+aHeight), SexyVector2(x+aWidth, y) , SexyVector2(x+aWidth, y+aHeight) };

		int i;
		for (i=0; i<4; i++)
		{
			p[i] = mTransformStack.back()*p[i];
			p[i].x -= 0.5f;
			p[i].y -= 0.5f;
			aVertex[i][0] = p[i].x;
			aVertex[i][1] = p[i].y;
		}
	}

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_STRIP);
		for (int i=0; i<4; i++)
		{
			glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
			glVertex2f(aVertex[i][0], aVertex[i][1]);
		}
	glEnd();
}

void GLInterface::DrawTriangle(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);
	unsigned int col1 = GetColorFromTriVertex(p1, aColor);
	unsigned int col2 = GetColorFromTriVertex(p1, aColor);
	unsigned int col3 = GetColorFromTriVertex(p1, aColor);

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_STRIP);
		glColor4ubv((uint8_t*)&col1);
		glVertex2f(p1.x, p1.y);

		glColor4ubv((uint8_t*)&col2);
		glVertex2f(p2.x, p2.y);

		glColor4ubv((uint8_t*)&col3);
		glVertex2f(p3.x, p3.y);
	glEnd();
}

void GLInterface::DrawTriangleTex(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor, int theDrawMode, Image *theTexture, bool blend)
{
	TriVertex aVertices[1][3] = {{p1, p2, p3}};
	DrawTrianglesTex(aVertices,1,theColor,theDrawMode,theTexture,blend);
}

void GLInterface::DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor, int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	if (!PreDraw()) return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*)theTexture;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	SetLinearFilter(blend);

	unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);
	aData->BltTriangles(theVertices, theNumTriangles, aColor, tx, ty);
}

void GLInterface::DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor, int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	TriVertex aList[100][3];
	int aTriNum = 0;
	while (aTriNum < theNumTriangles)
	{
		int aMaxTriangles = std::min(100,theNumTriangles - aTriNum);
		for (int i=0; i<aMaxTriangles; i++)
		{
			aList[i][0] = theVertices[aTriNum];
			aList[i][1] = theVertices[aTriNum+1];
			aList[i][2] = theVertices[aTriNum+2];
			aTriNum++;
		}
		DrawTrianglesTex(aList,aMaxTriangles,theColor,theDrawMode,theTexture, tx, ty, blend);
	}
}

void GLInterface::FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor, int theDrawMode, int tx, int ty)
{
	if (theNumVertices<3)
		return;

	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);
	unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);

	glDisable(GL_TEXTURE_2D);

	VertexList aList;
	for (int i=0; i<theNumVertices; i++)
	{
		GLVertex vert = { {theVertices[i].mX + (float)tx}, {theVertices[i].mY + (float)ty}, {0}, {1}, {aColor}, {0}, {0}, {0} };
		if (!mTransformStack.empty())
		{
			SexyVector2 v(vert.sx,vert.sy);
			v = mTransformStack.back()*v;
			vert.sx = v.x;
			vert.sy = v.y;
		}

		aList.push_back(vert);
	}

	if (theClipRect != NULL)
		DrawPolyClipped(theClipRect,aList);
	else
	{
		glBegin(GL_TRIANGLE_FAN);
		for (unsigned i=0; i<aList.size(); i++)
		{
			GLVertex& v = aList[i];

			glColor4ubv((uint8_t*)&v.color);
			glTexCoord2f(v.tu, v.tv);
			glVertex2f(v.sx, v.sy);
		}
		glEnd();
	}
}

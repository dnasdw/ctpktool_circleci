#include "ctpk.h"
#include <png.h>
#include <PVRTextureUtilities.h>

const u32 CCtpk::s_uSignature = SDW_CONVERT_ENDIAN32('CTPK');
const int CCtpk::s_nBPP[] = { 32, 24, 16, 16, 16, 16, 16, 8, 8, 8, 4, 4, 4, 8 };
const int CCtpk::s_nDecodeTransByte[64] =
{
	 0,  1,  4,  5, 16, 17, 20, 21,
	 2,  3,  6,  7, 18, 19, 22, 23,
	 8,  9, 12, 13, 24, 25, 28, 29,
	10, 11, 14, 15, 26, 27, 30, 31,
	32, 33, 36, 37, 48, 49, 52, 53,
	34, 35, 38, 39, 50, 51, 54, 55,
	40, 41, 44, 45, 56, 57, 60, 61,
	42, 43, 46, 47, 58, 59, 62, 63
};

CCtpk::CCtpk()
	: m_bVerbose(false)
{
}

CCtpk::~CCtpk()
{
}

void CCtpk::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CCtpk::SetDirName(const UString& a_sDirName)
{
	m_sDirName = a_sDirName;
}

void CCtpk::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CCtpk::ExportFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	u32 uCtpkSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pCtpk = new u8[uCtpkSize];
	fread(pCtpk, 1, uCtpkSize, fp);
	fclose(fp);
	SCtpkHeader* pCtpkHeader = reinterpret_cast<SCtpkHeader*>(pCtpk);
	if (pCtpkHeader->Signature != s_uSignature)
	{
		delete[] pCtpk;
		return DecodeFile();
	}
	SCtrTextureInfo* pCtrTextureInfo = reinterpret_cast<SCtrTextureInfo*>(pCtpk + sizeof(SCtpkHeader));
	STextureShortInfo* pTextureShortInfo = reinterpret_cast<STextureShortInfo*>(pCtpk + pCtpkHeader->TextureShortInfoOffset);
	UMkdir(m_sDirName.c_str());
	for (n32 i = 0; i < pCtpkHeader->Count; i++)
	{
		if (pTextureShortInfo[i].TextFormat != 0xFF && pCtrTextureInfo[i].TexFormat != pTextureShortInfo[i].TextFormat)
		{
			bResult = false;
			UPrintf(USTR("ERROR: format is not equivalent\n\n"));
			break;
		}
		if (pCtrTextureInfo[i].TexFormat < kTextureFormatRGBA8888 || pCtrTextureInfo[i].TexFormat > kTextureFormatETC1_A4)
		{
			bResult = false;
			UPrintf(USTR("ERROR: unknown format %d\n\n"), pCtrTextureInfo[i].TexFormat);
			break;
		}
		n32 nCheckSize = 0;
		for (n32 l = 0; l < pCtrTextureInfo[i].MipLevel; l++)
		{
			n32 nMipmapHeight = pCtrTextureInfo[i].Height >> l;
			n32 nMipmapWidth = pCtrTextureInfo[i].Width >> l;
			nCheckSize += nMipmapHeight * nMipmapWidth * s_nBPP[pCtrTextureInfo[i].TexFormat] / 8;
		}
		if (pCtrTextureInfo[i].TexDataSize != nCheckSize && m_bVerbose)
		{
			UPrintf(USTR("INFO: width: %X, height: %X, checksize: %X, size: %X, bpp: %d, format: %0X\n"), pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, nCheckSize, pCtrTextureInfo[i].TexDataSize, pCtrTextureInfo[i].TexDataSize * 8 / pCtrTextureInfo[i].Width / pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat);
		}
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		if (decode(pCtpk + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat, &pPVRTexture) == 0)
		{
			UString sPngFileName = XToU(reinterpret_cast<char*>(pCtpk + pCtrTextureInfo[i].FilePathOffset), 932, "CP932");
			remove(sPngFileName.begin(), sPngFileName.end(), USTR(':'));
			vector<UString> vDirPath = SplitOf(sPngFileName, USTR("/\\"));
			UString sDirName = m_sDirName;
			for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
			{
				sDirName += USTR("/") + vDirPath[j];
				UMkdir(sDirName.c_str());
			}
			sPngFileName = sDirName + USTR("/") + vDirPath.back() + USTR(".png");
			FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("wb"));
			if (fpSub == nullptr)
			{
				delete pPVRTexture;
				bResult = false;
				break;
			}
			if (m_bVerbose)
			{
				UPrintf(USTR("save: %") PRIUS USTR("\n"), sPngFileName.c_str());
			}
			png_structp pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (pPng == nullptr)
			{
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: png_create_write_struct error\n\n"));
				break;
			}
			png_infop pInfo = png_create_info_struct(pPng);
			if (pInfo == nullptr)
			{
				png_destroy_write_struct(&pPng, nullptr);
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
				break;
			}
			if (setjmp(png_jmpbuf(pPng)) != 0)
			{
				png_destroy_write_struct(&pPng, &pInfo);
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: setjmp error\n\n"));
				break;
			}
			png_init_io(pPng, fpSub);
			png_set_IHDR(pPng, pInfo, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			u8* pData = static_cast<u8*>(pPVRTexture->getDataPtr());
			png_bytepp pRowPointers = new png_bytep[pCtrTextureInfo[i].Height];
			for (n32 j = 0; j < pCtrTextureInfo[i].Height; j++)
			{
				pRowPointers[j] = pData + j * pCtrTextureInfo[i].Width * 4;
			}
			png_set_rows(pPng, pInfo, pRowPointers);
			png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&pPng, &pInfo);
			delete[] pRowPointers;
			fclose(fpSub);
			delete pPVRTexture;
		}
		else
		{
			bResult = false;
			UPrintf(USTR("ERROR: decode error\n\n"));
			break;
		}
	}
	delete[] pCtpk;
	return bResult;
}

bool CCtpk::ImportFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	u32 uCtpkSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pCtpk = new u8[uCtpkSize];
	fread(pCtpk, 1, uCtpkSize, fp);
	fclose(fp);
	SCtpkHeader* pCtpkHeader = reinterpret_cast<SCtpkHeader*>(pCtpk);
	if (pCtpkHeader->Signature != s_uSignature)
	{
		delete[] pCtpk;
		return EncodeFile();
	}
	SCtrTextureInfo* pCtrTextureInfo = reinterpret_cast<SCtrTextureInfo*>(pCtpk + sizeof(SCtpkHeader));
	STextureShortInfo* pTextureShortInfo = reinterpret_cast<STextureShortInfo*>(pCtpk + pCtpkHeader->TextureShortInfoOffset);
	for (n32 i = 0; i < pCtpkHeader->Count; i++)
	{
		if (pTextureShortInfo[i].TextFormat != 0xFF && pCtrTextureInfo[i].TexFormat != pTextureShortInfo[i].TextFormat)
		{
			bResult = false;
			UPrintf(USTR("ERROR: format is not equivalent\n\n"));
			break;
		}
		if (pCtrTextureInfo[i].TexFormat < kTextureFormatRGBA8888 || pCtrTextureInfo[i].TexFormat > kTextureFormatETC1_A4)
		{
			bResult = false;
			UPrintf(USTR("ERROR: unknown format %d\n\n"), pCtrTextureInfo[i].TexFormat);
			break;
		}
		n32 nCheckSize = 0;
		for (n32 l = 0; l < pCtrTextureInfo[i].MipLevel; l++)
		{
			n32 nMipmapHeight = pCtrTextureInfo[i].Height >> l;
			n32 nMipmapWidth = pCtrTextureInfo[i].Width >> l;
			nCheckSize += nMipmapHeight * nMipmapWidth * s_nBPP[pCtrTextureInfo[i].TexFormat] / 8;
		}
		if (pCtrTextureInfo[i].TexDataSize != nCheckSize && m_bVerbose)
		{
			UPrintf(USTR("INFO: width: %X, height: %X, checksize: %X, size: %X, bpp: %d, format: %0X\n"), pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, nCheckSize, pCtrTextureInfo[i].TexDataSize, pCtrTextureInfo[i].TexDataSize * 8 / pCtrTextureInfo[i].Width / pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat);
		}
		UString sPngFileName = XToU(reinterpret_cast<char*>(pCtpk + pCtrTextureInfo[i].FilePathOffset), 932, "CP932");
		remove(sPngFileName.begin(), sPngFileName.end(), USTR(':'));
		vector<UString> vDirPath = SplitOf(sPngFileName, USTR("/\\"));
		UString sDirName = m_sDirName;
		for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
		{
			sDirName += USTR("/") + vDirPath[j];
		}
		sPngFileName = sDirName + USTR("/") + vDirPath.back() + USTR(".png");
		FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("rb"));
		if (fpSub == nullptr)
		{
			bResult = false;
			break;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), sPngFileName.c_str());
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_read_struct error\n\n"));
			break;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
			break;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
			break;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: setjmp error\n\n"));
			break;
		}
		png_init_io(pPng, fpSub);
		png_read_info(pPng, pInfo);
		n32 nPngWidth = png_get_image_width(pPng, pInfo);
		if (nPngWidth != pCtrTextureInfo[i].Width)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nPngWidth != Width\n\n"));
			break;
		}
		n32 nPngHeight = png_get_image_height(pPng, pInfo);
		if (nPngHeight != pCtrTextureInfo[i].Height)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nPngHeight != Height\n\n"));
			break;
		}
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nBitDepth != 8\n\n"));
			break;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nColorType != PNG_COLOR_TYPE_RGB_ALPHA\n\n"));
			break;
		}
		u8* pData = new u8[nPngWidth * nPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[nPngHeight];
		for (n32 j = 0; j < nPngHeight; j++)
		{
			pRowPointers[j] = pData + j * nPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fpSub);
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		bool bSame = decode(pCtpk + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat, &pPVRTexture) == 0 && memcmp(pPVRTexture->getDataPtr(), pData, pCtrTextureInfo[i].Width * pCtrTextureInfo[i].Height * 4) == 0;
		delete pPVRTexture;
		if (!bSame)
		{
			u8* pBuffer = nullptr;
			encode(pData, nPngWidth, nPngHeight, pCtrTextureInfo[i].TexFormat, pCtrTextureInfo[i].MipLevel, s_nBPP[pCtrTextureInfo[i].TexFormat], &pBuffer);
			memcpy(pCtpk + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pBuffer, pCtrTextureInfo[i].TexDataSize);
			delete[] pBuffer;
		}
		delete[] pData;
	}
	if (bResult)
	{
		fp = UFopen(m_sFileName.c_str(), USTR("wb"));
		if (fp != nullptr)
		{
			fwrite(pCtpk, 1, uCtpkSize, fp);
			fclose(fp);
		}
		else
		{
			bResult = false;
		}
	}
	delete[] pCtpk;
	return bResult;
}

bool CCtpk::DecodeFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	u32 uCtpkSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pCtpk = new u8[uCtpkSize];
	fread(pCtpk, 1, uCtpkSize, fp);
	fclose(fp);
	n32 nWidth = static_cast<n32>(sqrt(static_cast<double>(uCtpkSize / 2)));
	n32 nHeight = nWidth;
	UMkdir(m_sDirName.c_str());
	do
	{
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		if (decode(pCtpk, nWidth, nHeight, kTextureFormatRGB565, &pPVRTexture) == 0)
		{
			vector<UString> vDirPath = SplitOf(m_sDirName, USTR("/\\"));
			UString sPngFileName = m_sDirName + USTR("/") + vDirPath.back() + USTR(".png");
			FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("wb"));
			if (fpSub == nullptr)
			{
				delete pPVRTexture;
				bResult = false;
				break;
			}
			if (m_bVerbose)
			{
				UPrintf(USTR("save: %") PRIUS USTR("\n"), sPngFileName.c_str());
			}
			png_structp pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (pPng == nullptr)
			{
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: png_create_write_struct error\n\n"));
				break;
			}
			png_infop pInfo = png_create_info_struct(pPng);
			if (pInfo == nullptr)
			{
				png_destroy_write_struct(&pPng, nullptr);
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
				break;
			}
			if (setjmp(png_jmpbuf(pPng)) != 0)
			{
				png_destroy_write_struct(&pPng, &pInfo);
				fclose(fpSub);
				delete pPVRTexture;
				bResult = false;
				UPrintf(USTR("ERROR: setjmp error\n\n"));
				break;
			}
			png_init_io(pPng, fpSub);
			png_set_IHDR(pPng, pInfo, nWidth, nHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			u8* pData = static_cast<u8*>(pPVRTexture->getDataPtr());
			png_bytepp pRowPointers = new png_bytep[nHeight];
			for (n32 j = 0; j < nHeight; j++)
			{
				pRowPointers[j] = pData + j * nWidth * 4;
			}
			png_set_rows(pPng, pInfo, pRowPointers);
			png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&pPng, &pInfo);
			delete[] pRowPointers;
			fclose(fpSub);
			delete pPVRTexture;
		}
		else
		{
			bResult = false;
			UPrintf(USTR("ERROR: decode error\n\n"));
			break;
		}
	} while (false);
	delete[] pCtpk;
	return bResult;
}

bool CCtpk::EncodeFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	u32 uCtpkSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pCtpk = new u8[uCtpkSize];
	fread(pCtpk, 1, uCtpkSize, fp);
	fclose(fp);
	n32 nWidth = static_cast<n32>(sqrt(static_cast<double>(uCtpkSize / 2)));
	n32 nHeight = nWidth;
	do
	{
		vector<UString> vDirPath = SplitOf(m_sDirName, USTR("/\\"));
		UString sPngFileName = m_sDirName + USTR("/") + vDirPath.back() + USTR(".png");
		FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("rb"));
		if (fpSub == nullptr)
		{
			bResult = false;
			break;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), sPngFileName.c_str());
		}
		png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (pPng == nullptr)
		{
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_read_struct error\n\n"));
			break;
		}
		png_infop pInfo = png_create_info_struct(pPng);
		if (pInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, nullptr, nullptr);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
			break;
		}
		png_infop pEndInfo = png_create_info_struct(pPng);
		if (pEndInfo == nullptr)
		{
			png_destroy_read_struct(&pPng, &pInfo, nullptr);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: png_create_info_struct error\n\n"));
			break;
		}
		if (setjmp(png_jmpbuf(pPng)) != 0)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: setjmp error\n\n"));
			break;
		}
		png_init_io(pPng, fpSub);
		png_read_info(pPng, pInfo);
		n32 nPngWidth = png_get_image_width(pPng, pInfo);
		if (nPngWidth != nWidth)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nPngWidth != nWidth\n\n"));
			break;
		}
		n32 nPngHeight = png_get_image_height(pPng, pInfo);
		if (nPngHeight != nHeight)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nPngHeight != nHeight\n\n"));
			break;
		}
		n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nBitDepth != 8\n\n"));
			break;
		}
		n32 nColorType = png_get_color_type(pPng, pInfo);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
			fclose(fpSub);
			bResult = false;
			UPrintf(USTR("ERROR: nColorType != PNG_COLOR_TYPE_RGB_ALPHA\n\n"));
			break;
		}
		u8* pData = new u8[nPngWidth * nPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[nPngHeight];
		for (n32 j = 0; j < nPngHeight; j++)
		{
			pRowPointers[j] = pData + j * nPngWidth * 4;
		}
		png_read_image(pPng, pRowPointers);
		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
		delete[] pRowPointers;
		fclose(fpSub);
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		bool bSame = decode(pCtpk, nWidth, nHeight, kTextureFormatRGB565, &pPVRTexture) == 0 && memcmp(pPVRTexture->getDataPtr(), pData, nWidth * nHeight * 4) == 0;
		delete pPVRTexture;
		if (!bSame)
		{
			u8* pBuffer = nullptr;
			encode(pData, nPngWidth, nPngHeight, kTextureFormatRGB565, 1, s_nBPP[kTextureFormatRGB565], &pBuffer);
			memcpy(pCtpk, pBuffer, uCtpkSize);
			delete[] pBuffer;
		}
		delete[] pData;
	} while (false);
	if (bResult)
	{
		fp = UFopen(m_sFileName.c_str(), USTR("wb"));
		if (fp != nullptr)
		{
			fwrite(pCtpk, 1, uCtpkSize, fp);
			fclose(fp);
		}
		else
		{
			bResult = false;
		}
	}
	delete[] pCtpk;
	return bResult;
}

bool CCtpk::IsCtpkFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SCtpkHeader ctpkHeader;
	fread(&ctpkHeader, sizeof(SCtpkHeader), 1, fp);
	fclose(fp);
	return ctpkHeader.Signature == s_uSignature;
}

bool CCtpk::IsCtpkIconFile(const UString& a_sFileName)
{
	n64 nCtpkSize = 0;
	if (!UGetFileSize(a_sFileName.c_str(), nCtpkSize))
	{
		UPrintf(USTR("ERROR: get %") PRIUS USTR(" size error\n\n"), a_sFileName.c_str());
		return false;
	}
	if (nCtpkSize <= 0 || nCtpkSize % 2 != 0)
	{
		return false;
	}
	nCtpkSize /= 2;
	n32 nSize = static_cast<n32>(sqrt(static_cast<double>(nCtpkSize)));
	return nSize * nSize == nCtpkSize && nSize % 8 == 0;
}

int CCtpk::decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture)
{
	u8* pRGBA = nullptr;
	u8* pAlpha = nullptr;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pTemp[(i * 64 + j) * 4 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pRGBA[(i * a_nWidth + j) * 4 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGB888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pTemp[(i * 64 + j) * 3 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pRGBA[(i * a_nWidth + j) * 3 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGBA5551:
	case kTextureFormatRGB565:
	case kTextureFormatRGBA4444:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatLA88:
	case kTextureFormatHL8:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL8:
	case kTextureFormatA8:
	case kTextureFormatLA44:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					pTemp[i * 64 + j] = a_pBuffer[i * 64 + s_nDecodeTransByte[j]];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL4:
	case kTextureFormatA4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j += 2)
				{
					pTemp[i * 64 + j] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] & 0xF) * 0x11;
					pTemp[i * 64 + j + 1] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] >> 4 & 0xF) * 0x11;
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[i * 8 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1_A4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[8 + i * 16 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
			pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 16; i++)
			{
				for (n32 j = 0; j < 4; j++)
				{
					pTemp[i * 16 + j] = (a_pBuffer[i * 16 + j * 2] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 4] = (a_pBuffer[i * 16 + j * 2] >> 4 & 0x0F) * 0x11;
					pTemp[i * 16 + j + 8] = (a_pBuffer[i * 16 + j * 2 + 1] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 12] = (a_pBuffer[i * 16 + j * 2 + 1] >> 4 & 0x0F) * 0x11;
				}
			}
			pAlpha = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pAlpha[i * a_nWidth + j] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4];
				}
			}
			delete[] pTemp;
		}
		break;
	}
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	case kTextureFormatETC1_A4:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	}
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	*a_pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBA);
	delete[] pRGBA;
	pvrtexture::Transcode(**a_pPVRTexture, pvrtexture::PVRStandard8PixelType, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		u8* pData = static_cast<u8*>((*a_pPVRTexture)->getDataPtr());
		for (n32 i = 0; i < a_nHeight; i++)
		{
			for (n32 j = 0; j < a_nWidth; j++)
			{
				pData[(i * a_nWidth + j) * 4 + 3] = pAlpha[i * a_nWidth + j];
			}
		}
		delete[] pAlpha;
	}
	return 0;
}

void CCtpk::encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer)
{
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PVRStandard8PixelType.PixelTypeID;
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	pvrtexture::CPVRTexture* pPVRTexture = nullptr;
	pvrtexture::CPVRTexture* pPVRTextureAlpha = nullptr;
	if (a_nFormat != kTextureFormatETC1_A4)
	{
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, a_pData);
	}
	else
	{
		u8* pRGBAData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pRGBAData, a_pData, a_nWidth * a_nHeight * 4);
		u8* pAlphaData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pAlphaData, a_pData, a_nWidth * a_nHeight * 4);
		for (n32 i = 0; i < a_nWidth * a_nHeight; i++)
		{
			pRGBAData[i * 4 + 3] = 0xFF;
			pAlphaData[i * 4] = 0;
			pAlphaData[i * 4 + 1] = 0;
			pAlphaData[i * 4 + 2] = 0;
		}
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBAData);
		pPVRTextureAlpha = new pvrtexture::CPVRTexture(pvrTextureHeader, pAlphaData);
		delete[] pRGBAData;
		delete[] pAlphaData;
	}
	if (a_nMipmapLevel != 1)
	{
		pvrtexture::GenerateMIPMaps(*pPVRTexture, pvrtexture::eResizeNearest, a_nMipmapLevel);
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pvrtexture::GenerateMIPMaps(*pPVRTextureAlpha, pvrtexture::eResizeNearest, a_nMipmapLevel);
		}
	}
	pvrtexture::uint64 uPixelFormat = 0;
	pvrtexture::ECompressorQuality eCompressorQuality = pvrtexture::ePVRTCBest;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	case kTextureFormatETC1_A4:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	}
	pvrtexture::Transcode(*pPVRTexture, uPixelFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eCompressorQuality);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		pvrtexture::Transcode(*pPVRTextureAlpha, pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, pvrtexture::ePVRTCBest);
	}
	n32 nTotalSize = 0;
	n32 nCurrentSize = 0;
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		nTotalSize += (a_nWidth >> l) * (a_nHeight >> l) * a_nBPP / 8;
	}
	*a_pBuffer = new u8[nTotalSize];
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		n32 nMipmapWidth = a_nWidth >> l;
		n32 nMipmapHeight = a_nHeight >> l;
		u8* pRGBA = static_cast<u8*>(pPVRTexture->getDataPtr(l));
		u8* pAlpha = nullptr;
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pAlpha = static_cast<u8*>(pPVRTextureAlpha->getDataPtr(l));
		}
		switch (a_nFormat)
		{
		case kTextureFormatRGBA8888:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 4];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k] = pRGBA[(i * nMipmapWidth + j) * 4 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k] = pTemp[(i * 64 + j) * 4 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGB888:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 3];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k] = pRGBA[(i * nMipmapWidth + j) * 3 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k] = pTemp[(i * 64 + j) * 3 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGBA5551:
		case kTextureFormatRGB565:
		case kTextureFormatRGBA4444:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatLA88:
		case kTextureFormatHL8:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL8:
		case kTextureFormatA8:
		case kTextureFormatLA44:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						pMipmapBuffer[i * 64 + s_nDecodeTransByte[j]] = pTemp[i * 64 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL4:
		case kTextureFormatA4:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j += 2)
					{
						pMipmapBuffer[i * 32 + s_nDecodeTransByte[j] / 2] = ((pTemp[i * 64 + j] / 0x11) & 0x0F) | ((pTemp[i * 64 + j + 1] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
				for (n32 i = 0; i < nMipmapHeight; i += 4)
				{
					for (n32 j = 0; j < nMipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[i * 8 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1_A4:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
				for (n32 i = 0; i < nMipmapHeight; i += 4)
				{
					for (n32 j = 0; j < nMipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[8 + i * 16 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
				pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4] = pAlpha[i * nMipmapWidth + j];
					}
				}
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 16; i++)
				{
					for (n32 j = 0; j < 4; j++)
					{
						pMipmapBuffer[i * 16 + j * 2] = ((pTemp[i * 16 + j] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 4] / 0x11) << 4 & 0xF0);
						pMipmapBuffer[i * 16 + j * 2 + 1] = ((pTemp[i * 16 + j + 8] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 12] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		}
		nCurrentSize += nMipmapWidth * nMipmapHeight * a_nBPP / 8;
	}
	delete pPVRTexture;
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		delete pPVRTextureAlpha;
	}
}

#include "wingdiputils.h"

std::wstring to_wide(const std::string& multi) {
	std::wstring wide; wchar_t w; mbstate_t mb{};
	size_t n = 0, len = multi.length() + 1;
	while (auto res = mbrtowc(&w, multi.c_str() + n, len - n, &mb)) {
		if (res == size_t(-1) || res == size_t(-2))
			throw "invalid encoding";

		n += res;
		wide += w;
	}
	return wide;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

bool GdiplusErrorHandler(Gdiplus::Status& res) {
	switch (res) {
	case Gdiplus::Status::Ok:
		std::cout << "[Info] Success" << std::endl;
		return false;
	case Gdiplus::Status::GenericError:
		std::cerr << "[Error] Error Saving File: Unknown Error" << std::endl;
		return true;
	case Gdiplus::Status::InvalidParameter:
		std::cerr << "[Error] Error Saving File: Invalid Parameter" << std::endl;
		return true;
	case Gdiplus::Status::OutOfMemory:
		std::cerr << "[Error] Error Saving File: Out Of Memory" << std::endl;
		return true;
	case Gdiplus::Status::ObjectBusy:
		std::cerr << "[Error] Error Saving File: Object Busy" << std::endl;
		return true;
	case Gdiplus::Status::InsufficientBuffer:
		std::cerr << "[Error] Error Saving File: Insufficient Buffer" << std::endl;
		return true;
	case Gdiplus::Status::NotImplemented:
		std::cerr << "[Error] Error Saving File: Not Implemented" << std::endl;
		return true;
	case Gdiplus::Status::Win32Error:
		std::cerr << "[Error] Error Saving File: Win32 Error" << std::endl;
		return true;
	case Gdiplus::Status::WrongState:
		std::cerr << "[Error] Error Saving File: Wrong State" << std::endl;
		return true;
	case Gdiplus::Status::Aborted:
		std::cerr << "[Error] Error Saving File: Aborted" << std::endl;
		return true;
	case Gdiplus::Status::FileNotFound:
		std::cerr << "[Error] Error Saving File: File Not Found" << std::endl;
		return true;
	case Gdiplus::Status::ValueOverflow:
		std::cerr << "[Error] Error Saving File: Value Overflow" << std::endl;
		return true;
	case Gdiplus::Status::AccessDenied:
		std::cerr << "[Error] Error Saving File: Access Denied" << std::endl;
		return true;
	case Gdiplus::Status::UnknownImageFormat:
		std::cerr << "[Error] Error Saving File: Unknown Image Format" << std::endl;
		return true;
	case Gdiplus::Status::FontFamilyNotFound:
		std::cerr << "[Error] Error Saving File: Font Family Not Found" << std::endl;
		return true;
	case Gdiplus::Status::FontStyleNotFound:
		std::cerr << "[Error] Error Saving File: Font Style Not Found" << std::endl;
		return true;
	case Gdiplus::Status::NotTrueTypeFont:
		std::cerr << "[Error] Error Saving File: Not True Type Font" << std::endl;
		return true;
	case Gdiplus::Status::UnsupportedGdiplusVersion:
		std::cerr << "[Error] Error Saving File: Unsupported GDI+ Version" << std::endl;
		return true;
	case Gdiplus::Status::GdiplusNotInitialized:
		std::cerr << "[Error] Error Saving File: GDI+ Not Initialized" << std::endl;
		return true;
	case Gdiplus::Status::PropertyNotFound:
		std::cerr << "[Error] Error Saving File: Property Not Found" << std::endl;
		return true;
	case Gdiplus::Status::PropertyNotSupported:
		std::cerr << "[Error] Error Saving File: Property Not Supported" << std::endl;
		return true;
	}
}

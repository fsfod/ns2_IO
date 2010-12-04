
class C7ZipFormatInfo{
public:
	C7ZipFormatInfo(){}
	~C7ZipFormatInfo(){}

public:
	std::wstring m_Name;
	GUID m_ClassID;
	bool m_UpdateEnabled;
	bool m_KeepName;

	std::vector<PathString> Exts;
	std::vector<PathString> AddExts;

	int FormatIndex;
};
#pragma once

//we can't just have non const ref parameters since luabind will treat them as return values
#define ConvertAndValidatePath(ArgName) const_cast<PathStringArg&>(ArgName).ConvertAndValidate(#ArgName)

class PathStringArg{
	public:
		PathStringArg(const char* String, int Length) :s(String), len(Length) {}
		~PathStringArg(void);
		 
		const PlatformString& PathStringArg::ConvertAndValidate(const char* VaribleName);

		operator const std::wstring&() const{ 
			return CachedConvertedString;
		}


		std::string ToString() const{
			return std::string(s, len);
		}

		std::string GetNormalizedPath() const;
		const PlatformString& GetConvertedNativePath()const;
		PlatformPath CreatePath(const PlatformPath& Root)const;
		PlatformPath CreateSearchPath(const PlatformPath& Root, const PathStringArg& SearchPath)const;
private:
		PlatformString CachedConvertedString;

		const char* s;
		const int len;
};


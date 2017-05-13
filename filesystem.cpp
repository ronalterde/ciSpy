#include "filesystem.h"

namespace filesystem {

using namespace std;

std::unique_ptr<std::istream> FileStreamFactory::makeInputStream(const std::string& path) {
	return make_unique<std::ifstream>(path);
}

std::unique_ptr<std::ostream> FileStreamFactory::makeOutputStream(const std::string& path) {
	return make_unique<std::ofstream>(path);
}

FileStore::FileStore(StreamFactory& streamFactory, const std::string& path) :
	streamFactory(streamFactory),
	path(path) {
}

void FileStore::set(const std::string& key, const std::string& value) {
	map<string, string> entireFile = readEntireFile();
	entireFile[key] = value;
	writeEntireFile(entireFile);
}

std::string FileStore::get(const std::string& key) {
	return readFirstOccurenceOf(key);
}

bool FileStore::beginsWith(const std::string& line, const std::string& key) {
	return (line.compare(0, key.size(), key) == 0);
}

std::string FileStore::extractPartAfterDelimiter(const std::string& line) {
	std::string::size_type found = line.find(":");
	   if (found == std::string::npos)
		   return "";
	   return line.substr(found + 1, line.size());
}

std::pair<std::string, std::string> FileStore::splitIntoKeyAndValue(const std::string& line) {
	std::string::size_type found = line.find(":");
	if (found == std::string::npos)
		return std::make_pair("", "");
	return std::make_pair(line.substr(0, found),
			line.substr(found + 1, line.size()));
}

std::map<std::string, std::string> FileStore::readEntireFile() {
	std::map<std::string, std::string> entireFile;
	auto inStream = streamFactory.makeInputStream(path);
	while (!inStream->eof()) {
		inStream->getline(line, MAX_LINE_LEN);
		auto keyAndValue = splitIntoKeyAndValue(string(line));
		if (keyAndValue.first == "" && keyAndValue.second == "")
			continue;
		entireFile.insert(keyAndValue);
	}
	return entireFile;
}

void FileStore::writeEntireFile(std::map<std::string, std::string> entireFile) {
	auto outStream = streamFactory.makeOutputStream(path);
	for (auto const& x : entireFile)
	{
		string a = x.first;
		string b = x.second;

		*outStream << x.first << ":" << x.second << endl;
	}	
}

std::string FileStore::readFirstOccurenceOf(const std::string& key) {
	auto stream = streamFactory.makeInputStream(path);
	while (!stream->eof()) {
		stream->getline(line, MAX_LINE_LEN);
		auto lineStr = string(line);
		if (beginsWith(lineStr, key)) {
			return extractPartAfterDelimiter(lineStr);
		}
	}
	return "";
}

} // namespace filesystem

#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <map>
#include <memory>
#include <fstream>

#include "common.h"

namespace filesystem {

class StreamFactory {
public:
	virtual ~StreamFactory() {}
	virtual std::unique_ptr<std::istream> makeInputStream(const std::string& path) = 0;

	/*
	 * Client assumes empty file.
	 */
	virtual std::unique_ptr<std::ostream> makeOutputStream(const std::string& path) = 0;
};

class FileStreamFactory : public StreamFactory {
public:
	std::unique_ptr<std::istream> makeInputStream(const std::string& path);
	std::unique_ptr<std::ostream> makeOutputStream(const std::string& path);
};

class FileStore : public common::KeyValueStore {
public:
	FileStore(StreamFactory& streamFactory, const std::string& path);
	void set(const std::string& key, const std::string& value);
	std::string get(const std::string& key);
	
private:
	static bool beginsWith(const std::string& line, const std::string& key);
	static std::string extractPartAfterDelimiter(const std::string& line);
	static std::pair<std::string, std::string> splitIntoKeyAndValue(
			const std::string& line);
	std::map<std::string, std::string> readEntireFile();
	void writeEntireFile(std::map<std::string, std::string> entireFile);
	std::string readFirstOccurenceOf(const std::string& key);

private:
	static const size_t MAX_LINE_LEN{500};
	StreamFactory& streamFactory;
	std::string path;
	char line[MAX_LINE_LEN];
};
} // namespace filesystem

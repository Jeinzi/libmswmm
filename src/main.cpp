#include <utf.h>
#include <memory>
#include <fstream>
#include <iostream>

#include <compoundfilereader.h>


CFB::COMPOUND_FILE_ENTRY const* findStream(CFB::CompoundFileReader const& reader, char const* streamName) {
  CFB::COMPOUND_FILE_ENTRY const* ret = nullptr;
  reader.EnumFiles(reader.GetRootEntry(), -1, [&](CFB::COMPOUND_FILE_ENTRY const* entry,
                                                  CFB::utf16string const& u16dir,
                                                  int level)
                                                  ->void
  {
    if (reader.IsStream(entry)) {
      std::string name = UTF16ToUTF8(entry->name);
      if (u16dir.length() > 0) {
        std::string dir = UTF16ToUTF8(u16dir.c_str());
        if (strncmp(streamName, dir.c_str(), dir.length()) == 0 &&
            streamName[dir.length()] == '\\' &&
            strcmp(streamName + dir.length() + 1, name.c_str()) == 0)
        {
          ret = entry;
        }
      }
      else {
        if (strcmp(streamName, name.c_str()) == 0) {
          ret = entry;
        }
      }
    }
  });
  return ret;
}



int main() {
  std::string filename = "example-projects/6.0.6000.16386/empty-with-metadata.MSWMM";
  std::ifstream file(filename, std::ios_base::in | std::ios_base::binary);
  if (!file.good()) {
    std::cout << "Can't open file." << std::endl;
    return 1;
  }

  // Get file size.
  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);

  auto buffer = std::make_unique<char[]>(length);
  file.read(buffer.get(), length);
  try {
    CFB::CompoundFileReader reader(buffer.get(), length);
    auto xmlStream = findStream(reader, "ProducerData\\Producer.Dat");
    if (!xmlStream) {
      std::cout << "Can't find project data in file." << std::endl;
      return 1;
    }
    auto xmlBuffer = std::make_unique<char[]>(xmlStream->size);
    reader.ReadFile(xmlStream, 0, xmlBuffer.get(), xmlStream->size);

    std::cout << "MSWMM project definition:\n";
    std::cout << std::string(xmlBuffer.get(), xmlStream->size) << std::endl;
  }
  catch (CFB::WrongFormat& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }


  return 0;
}

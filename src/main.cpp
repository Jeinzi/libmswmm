#include <memory>
#include <fstream>
#include <iostream>

#include <QtXml/QDomDocument>

#include "utf.h"
#include "compoundfilereader.h"



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
  // Open file.
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

  // Read file into buffer.
  auto buffer = std::make_unique<char[]>(length);
  file.read(buffer.get(), length);
  // Create XML DOM.
  QDomDocument xmlDoc("mswmm-project");
  try {
    // Parse CFB file.
    CFB::CompoundFileReader reader(buffer.get(), length);

    // Get XML file defining the MSWMM project.
    auto xmlStream = findStream(reader, "ProducerData\\Producer.Dat");
    if (!xmlStream) {
      std::cout << "Can't find project data in file." << std::endl;
      return 1;
    }
    // The following code assumes that the buffer contains UTF-16 text.
    if (xmlStream->size % 2 != 0) {
      std::cout << "Project XML is unexpectedly not encoded as UTF-16. Parsing stopped." << std::endl;
      return 1;
    }
    // Read XML into buffer.
    auto xmlBuffer = std::make_unique<char[]>(xmlStream->size+2);
    reader.ReadFile(xmlStream, 0, xmlBuffer.get(), xmlStream->size);
    // Add a zero termination for good measure, so
    // QString::fromUtf16() definitely terminates.
    xmlBuffer[xmlStream->size] = 0;
    xmlBuffer[xmlStream->size+1] = 0;
    auto xmlString = QString::fromUtf16((char16_t*)xmlBuffer.get()).trimmed();

    // Parse XML.
    QString errorStr;
    int errorLine;
    int errorCol;
    if (!xmlDoc.setContent(xmlString, false, &errorStr, &errorLine, &errorCol)) {
      std::cout << "Can't parse project definition XML." << std::endl;
      std::cout << errorStr.toStdString() << std::endl;
      std::cout << "Pos: " << errorLine << "|" << errorCol << std::endl;
      return 1;
    }
  }
  catch (CFB::WrongFormat& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }


  // Extract data from XML DOM.
  auto xmlRoot = xmlDoc.documentElement();
  auto dataStr = xmlRoot.firstChildElement("Project")
                        .firstChildElement("DataStr");
  auto producerProperties = dataStr.firstChildElement("ProducerProperties");

  std::cout << "Metadata:\n";
  QDomNode n = producerProperties.firstChildElement("MetDat");
  while (!n.isNull()) {
    auto attr = n.attributes();
    std::string key   = attr.namedItem("MDTag").nodeValue().toStdString();
    std::string value = attr.namedItem("MDVal").nodeValue().toStdString();
    std::cout << "  " << key << ": " << value << "\n";
    n = n.nextSiblingElement("MetDat");
  }

  std::cout << "Files used in project:\n";
  n = dataStr.firstChildElement("FileInfo");
  while (!n.isNull()) {
    auto fileInfo = n.attributes();
    std::string src = fileInfo.namedItem("SrceFn").nodeValue().toStdString();
    std::cout << "  " << src << "\n";
    n = n.nextSiblingElement("FileInfo");
  }


  return 0;
}

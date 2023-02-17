# Notes About MSWMM File Format
The MSWMM file is a CFB (Compound File Binary) file. It contains thumbnails, links; and the actual project definition in an XML at "ProducerData\\Producer.Dat".

## Adding an image to the video timeline (empty-with-metadata.MSWMM -> image-5s.MSWMM)
- The CFB container gets a new Thumbnail and a new ShellLink file
- Regarding the XML
  - The DataStr tag with its HOID, FileHigh and DocumentGuid tags changes
  - The TIArr tag gets a UID child element pointing to a TmlnStillItem tag
  - Also AVSource, Thmb, ClipStill and FileInfo tags get added

## General
- AVSource FileSize attribute gives the file size in kibibytes, with the decimal places truncated

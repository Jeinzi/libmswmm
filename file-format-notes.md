# Notes About MSWMM File Format
The MSWMM file is a CFB (Compound File Binary) file. It contains thumbnails, links; and the actual project definition in an XML at "ProducerData\\Producer.Dat".

## Adding an image to the video timeline (empty-with-metadata.MSWMM -> image-5s.MSWMM)
- The CFB container gets a new Thumbnail and a new ShellLink file
- Regarding the XML
  - The DataStr tag with its HOID, FileHigh and DocumentGuid tags changes
  - The TIArr tag gets a UID child element pointing to a TmlnStillItem tag
  - Also AVSource, Thmb, ClipStill and FileInfo tags get added

## General
- AVSource FileSize attribute gives the file size in kibibyte, with the decimal places truncated
- [MSDN documentation](https://web.archive.org/web/20081220175229/http://msdn.microsoft.com/en-us/library/bb288385(VS.85).aspx) on Windows Movie Maker XML for effects and transitions
- [Documentation](https://web.archive.org/web/20230121061014/http://www.rehanfx.org/customtc.htm) on custom effects and transitions
- [Windows Movie Maker forum](https://web.archive.org/web/20191024065123/http://windowsmoviemakers.net/Forums/ShowPost.aspx?PostID=2203) with tons of information

function get_files
{
    echo kerfuffle.xml
}

function po_for_file
{
    case "$1" in
       kerfuffle.xml)
           echo ark_xml_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       kerfuffle.xml)
           echo comment
       ;;
    esac
}

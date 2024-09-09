inputFile="combined_images_v4.h"
tempFile=$(mktemp)
num=0

cat img-0.h > "$inputFile"
cat img-1.h >> "$inputFile"
cat img-2.h >> "$inputFile"
cat img-3.h >> "$inputFile"
cat img-4.h >> "$inputFile"
cat img-5.h >> "$inputFile"
cat img-6.h >> "$inputFile"
cat img-7.h >> "$inputFile"
cat img-8.h >> "$inputFile"
cat img-9.h >> "$inputFile"
cat img-10.h >> "$inputFile"
cat img-11.h >> "$inputFile"
cat img-12.h >> "$inputFile"
cat img-13.h >> "$inputFile"
cat img-14.h >> "$inputFile"
cat img-15.h >> "$inputFile"
cat img-16.h >> "$inputFile"
cat img-17.h >> "$inputFile"
cat img-18.h >> "$inputFile"
cat img-19.h >> "$inputFile"
cat img-20.h >> "$inputFile"
cat img-21.h >> "$inputFile"
cat img-22.h >> "$inputFile"
cat img-23.h >> "$inputFile"
cat img-24.h >> "$inputFile"
cat img-25.h >> "$inputFile"
cat img-26.h >> "$inputFile"
cat img-27.h >> "$inputFile"

#awk '{gsub(/MagickImage/, "Frame" num); num++} 1' num=1 "$inputFile" > "$tempFile" && mv "$tempFile" "$inputFile"
#awk '{n=gsub(/MagickImage/,"Frame" num);num++ ) $0=gensub(/MagickImage/,"Frame"i,1)}1' $inputfile
#awk -v w='MagickImage' '{while($0~w) sub(w,"Frame"++c)}1' "$inputfile"
#awk -v RS='MagickImage' -v ORS='' 'NR>1 && $0="Frame"++c $0' "$inputfile"



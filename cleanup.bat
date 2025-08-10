# Remove UTF-8 BOM if present
$files = @("vehicle_fullfat_afentity.patch","vehicle_fullfat_player.patch")
foreach ($f in $files) {
  $bytes = [System.IO.File]::ReadAllBytes($f)
  if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    [System.IO.File]::WriteAllBytes($f, $bytes[3..($bytes.Length-1)])
  }
}

# Convert CRLF to LF (git apply prefers LF)
foreach ($f in $files) {
  (Get-Content $f -Raw) -replace "`r`n","`n" | Set-Content $f -NoNewline
}
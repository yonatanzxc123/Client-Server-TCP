<#
.SYNOPSIS
    Send ONE predefined HTTP request to the server, perfect for Wireshark captures.

.PARAMETER Case
    Name of the request to run.
    If omitted, the script shows a menu and waits for your selection.

.NOTES
    Default server is http://localhost:27015.  Change $baseUrl below if needed.
#>

param(
    [ValidateSet('OPTIONS','ROOTGET','HEAD','LANGHE','POST','PUT','GETFILE','DELETE','TRACE')]
    [string]$Case
)

$baseUrl = 'http://localhost:27015'

# Definition of all single-step test cases
$tests = @(
    @{ Key='OPTIONS'; Method = 'OPTIONS'; Uri = "$baseUrl/";           Body = $null            },
    @{ Key='ROOTGET'; Method = 'GET';     Uri = "$baseUrl/";           Body = $null            },
    @{ Key='HEAD';    Method = 'HEAD';    Uri = "$baseUrl/";           Body = $null            },
    @{ Key='LANGHE';  Method = 'GET';     Uri = "$baseUrl/?lang=he";   Body = $null            },
    @{ Key='POST';    Method = 'POST';    Uri = "$baseUrl/";           Body = 'Hello from test'},
    @{ Key='PUT';     Method = 'PUT';     Uri = "$baseUrl/test.txt";   Body = 'PUT test data'  },
    @{ Key='GETFILE'; Method = 'GET';     Uri = "$baseUrl/test.txt";   Body = $null            },
	@{ Key='DELETE';  Method='DELETE';  Uri="$baseUrl/test.txt";   Body=$null            },
    @{ Key='TRACE';   Method = 'TRACE';   Uri = "$baseUrl/";           Body = $null            }
)

# Interactive menu if -Case wasn’t provided
if (-not $Case) {
    Write-Host "`nSelect the request to send:`n" -ForegroundColor Cyan
    $i = 1
    foreach ($t in $tests) {
        Write-Host ("  {0,2}) {1,-7} {2}" -f $i, $t.Method, $t.Uri)
        $i++
    }
    $choice = Read-Host "`nEnter number"
    if (-not ($choice -as [int]) -or $choice -lt 1 -or $choice -gt $tests.Count) {
        Write-Error "Invalid selection." ; exit
    }
    $Case = $tests[$choice-1].Key
}

$test = $tests | Where-Object { $_.Key -eq $Case } | Select-Object -First 1
if (-not $test) { Write-Error "Unknown case '$Case'." ; exit }

Write-Host "`n--- Testing $($test.Method) $($test.Uri) ---" -ForegroundColor Cyan

try {
    $invoke = @{
        Method      = $test.Method
        Uri         = $test.Uri
        ErrorAction = 'Stop'
        UseBasicParsing = $true
    }
    if ($test.Body) {
        $invoke.Body = $test.Body
    }

    $resp = Invoke-WebRequest @invoke

    # Concise output for easy packet correlation
    Write-Host "Status : $($resp.StatusCode) $($resp.StatusDescription)"
    Write-Host "Content-Length : $($resp.Headers['Content-Length'])"
    if ($test.Method -notin @('HEAD','OPTIONS','TRACE')) {
        Write-Host "`nBody:`n$($resp.Content)`n"
    }
}
catch [System.Net.WebException] {
    $r = $_.Exception.Response
    if ($r -is [System.Net.HttpWebResponse]) {
        Write-Host "Status : $($r.StatusCode) $($r.StatusDescription)" -ForegroundColor Yellow
    } else {
        throw
    }
}
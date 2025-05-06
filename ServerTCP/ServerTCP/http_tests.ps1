# PowerShell script to test all HTTP methods against http://localhost:27015

$baseUrl = 'http://localhost:27015'

$tests = @(
    @{ Method = 'OPTIONS'; Uri = "$baseUrl/"; Body = $null },
    @{ Method = 'GET';     Uri = "$baseUrl/"; Body = $null },
    @{ Method = 'HEAD';    Uri = "$baseUrl/"; Body = $null },
    @{ Method = 'GET';     Uri = "$baseUrl/?lang=he"; Body = $null },
    @{ Method = 'POST';    Uri = "$baseUrl/"; Body = 'Hello from test' },
    @{ Method = 'PUT';     Uri = "$baseUrl/test.txt"; Body = 'PUT test data' },
    @{ Method = 'GET'; Uri = "$baseUrl/test.txt"; Body = $null },
    @{ Method = 'TRACE';   Uri = "$baseUrl/"; Body = $null }
)

foreach ($test in $tests) {
    Write-Host "--- Testing $($test.Method) $($test.Uri) ---" -ForegroundColor Cyan
    try {
        $invokeParams = @{
            Method      = $test.Method
            Uri         = $test.Uri
            ErrorAction = 'Stop'
        }
        if ($test.Body) {
            $invokeParams.Add('Body', $test.Body)
        }
        # Use Invoke-WebRequest to capture headers and content
        $response = Invoke-WebRequest @invokeParams

        # Output status and relevant headers
        Write-Host "Status: $($response.StatusCode) $($response.StatusDescription)"
        Write-Host "Content-Type: $($response.Headers['Content-Type'])"
        Write-Host "Content-Length: $($response.Headers['Content-Length'])"

        if ($test.Method -notin @('HEAD','OPTIONS','TRACE')) {
            Write-Host "Body:`n$($response.Content)`n"
        }
    }
    catch [System.Net.WebException] {
        $resp = $_.Exception.Response
        if ($resp -is [System.Net.HttpWebResponse]) {
            Write-Host "Status: $($resp.StatusCode) $($resp.StatusDescription)" -ForegroundColor Yellow
            $sr = New-Object System.IO.StreamReader($resp.GetResponseStream())
            Write-Host "Body:`n$($sr.ReadToEnd())`n" -ForegroundColor Yellow
        } else {
            Write-Error "Request failed: $_"
        }
    }
}

Write-Host 'All tests completed.' -ForegroundColor Green

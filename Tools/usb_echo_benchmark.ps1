param(
    [string]$Port = 'COM4',
    [int]$BaudRate = 115200,
    [int]$Bytes = 1048576,
    [int]$Runs = 3,
    [int]$WriteChunkBytes = 65536
)

$ErrorActionPreference = 'Stop'
if ($Bytes -le 0 -or $Runs -le 0) { throw 'Bytes and Runs must be positive.' }
if ($WriteChunkBytes -le 0) { throw 'WriteChunkBytes must be positive.' }

Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;

public class UsbEchoResult
{
    public int Bytes;
    public double Seconds;
    public double MegabytesPerSecond;
    public double MegabitsPerSecond;
    public bool Match;
}

public static class UsbEchoBenchmark
{
    public static UsbEchoResult Run(SerialPort port, int length, int writeChunk)
    {
        byte[] sent = new byte[length];
        byte[] received = new byte[length];
        new Random(2026).NextBytes(sent);

        // Keep host IN and OUT requests active while the device drains its queue.
        Stopwatch timer = Stopwatch.StartNew();
        int receivedCount = 0;
        Task reader = Task.Factory.StartNew(() => {
            try
            {
            while (receivedCount < length)
            {
                int count = port.Read(received, receivedCount, Math.Min(65536, length - receivedCount));
                if (count <= 0) throw new Exception("Read returned no data.");
                receivedCount += count;
            }
            }
            catch (TimeoutException error)
            {
                throw new TimeoutException("Received " + receivedCount + " of " + length + " bytes.", error);
            }
        }, CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
        try
        {
            for (int offset = 0; offset < length; offset += writeChunk)
                port.Write(sent, offset, Math.Min(writeChunk, length - offset));
        }
        catch
        {
            // Settle the reader before releasing the serial port.
            try { reader.GetAwaiter().GetResult(); } catch { }
            throw;
        }
        reader.GetAwaiter().GetResult();
        timer.Stop();

        bool match = true;
        for (int i = 0; i < length; i++)
            if (sent[i] != received[i]) { match = false; break; }
        double seconds = timer.Elapsed.TotalSeconds;
        return new UsbEchoResult {
            Bytes = length,
            Seconds = seconds,
            MegabytesPerSecond = length / seconds / 1000000.0,
            MegabitsPerSecond = length * 8.0 / seconds / 1000000.0,
            Match = match
        };
    }
}
'@

$echoPort = [System.IO.Ports.SerialPort]::new($Port, $BaudRate)
$echoPort.ReadTimeout = 3000
$echoPort.WriteTimeout = 5000
$echoPort.ReadBufferSize = 4194304
$echoPort.WriteBufferSize = 262144
$echoPort.DtrEnable = $true
try {
    $echoPort.Open()
    $echoPort.DiscardInBuffer()
    $echoPort.DiscardOutBuffer()
    foreach ($size in @(1, 64, 511, 512, 513, 1024, 32768, 32769, 65536, 65537)) {
        $check = [UsbEchoBenchmark]::Run($echoPort, $size, $WriteChunkBytes)
        if (-not $check.Match) { throw "Echo mismatch at $size bytes." }
    }
    Write-Output "Boundary and queue-wrap checks passed on $Port (1 through 65537 bytes)."
    for ($run = 1; $run -le $Runs; $run++) {
        $result = [UsbEchoBenchmark]::Run($echoPort, $Bytes, $WriteChunkBytes)
        if (-not $result.Match) { throw "Echo mismatch in run $run." }
        [pscustomobject]@{
            Run = $run
            Port = $Port
            Bytes = $result.Bytes
            WriteChunkBytes = $WriteChunkBytes
            Seconds = [Math]::Round($result.Seconds, 4)
            MegabytesPerSecond = [Math]::Round($result.MegabytesPerSecond, 3)
            MegabitsPerSecond = [Math]::Round($result.MegabitsPerSecond, 3)
            Match = $result.Match
        } | ConvertTo-Json -Compress
    }
}
finally {
    if ($echoPort.IsOpen) { $echoPort.Close() }
    $echoPort.Dispose()
}

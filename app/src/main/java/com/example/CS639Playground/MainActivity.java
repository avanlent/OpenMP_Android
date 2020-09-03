package com.example.CS639Playground;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;
import android.view.View;
import com.anychart.AnyChart;
import com.anychart.AnyChartView;
import com.anychart.chart.common.dataentry.DataEntry;
import com.anychart.chart.common.dataentry.ValueDataEntry;
import com.anychart.charts.Scatter;
import com.anychart.core.scatter.series.Marker;
import com.anychart.enums.MarkerType;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    // stuff for the UI
    private TextView consoleDisplay;
    private TextView countDisplay;
    private ScrollView scrollView;
    private Scatter scatter;
    private Marker marker;
    private AnyChartView chart;

    // keep track of things to send to C++
    private int thread_count = 1;
    private String[] modes = { "Cast", "PtrArr", "Random", "FlipXY" };
    private String[] modeCols = { "Red", "Green", "Blue", "Black" };
    private int mode = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        chart = findViewById(R.id.viewGraph);
        consoleDisplay = findViewById(R.id.consoleView);
        countDisplay = findViewById(R.id.txtCount);

        // setup for chart
        scatter = AnyChart.scatter();
        scatter.animation(false);  // no need for being fancy
        scatter.title("Time (ms) vs. Thread count");
        List<DataEntry> data = new ArrayList<>();
        data.add(new ValueDataEntry(0, 0));  // need some element for chart to start
        marker = scatter.marker(data);
        marker.type(MarkerType.CIRCLE).size(1);
        chart.setChart(scatter);
        scatter.removeAllSeries();  // remove the start element for clear chart


        // add listeners for buttons
        // execute button: calls c++ and lets it handle things from there
        final Button btnExecute = findViewById(R.id.btnExecute);
        btnExecute.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                executeCPP(mode);
            }
        });
        // plus button: increase the desired thread count, no limit
        final Button incre = findViewById(R.id.btnPlus);
        incre.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                MainActivity.this.thread_count++;
                MainActivity.this.countDisplay.setText(Integer.toString(MainActivity.this.thread_count));
            }
        });
        // minus button: decrease the desired thread count, n > 0
        final Button decre = findViewById(R.id.btnMinus);
        decre.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (MainActivity.this.thread_count > 1) {
                    MainActivity.this.thread_count--;
                    MainActivity.this.countDisplay.setText(Integer.toString(MainActivity.this.thread_count));
                }
            }
        });
        // cycle between the modes i.e tests for array layouts
        final Button btnMode = findViewById(R.id.btnMode);
        btnMode.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                btnMode.setText(modes[mode=(mode+1)%modes.length]);
            }
        });
        // clear button: clears the chart and consoleDisplay text
        final Button clr = findViewById(R.id.btnClear);
        clr.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        MainActivity.this.consoleDisplay.setText("");
                        MainActivity.this.scatter.removeAllSeries();
                    }
                });
            }
        });

        scrollView = (ScrollView) findViewById(R.id.scrollView);
    }

    // add data to the consoleDisplay and the chart
    public void displayData(double[] res, int t_cnt) {
        final List<DataEntry> graphData = new ArrayList<>(res.length);
        for(int i = 0; i < res.length; i ++){
            graphData.add(new ValueDataEntry(t_cnt, res[i]));
        }

        final double[] results = res;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MainActivity.this.consoleDisplay.append("\n");
                for (int i = 0; i < results.length; i++) {
                    MainActivity.this.consoleDisplay.append(Integer.toString(i+1) + " [" + Double.toString(results[i]) + "]\n");
                }
                MainActivity.this.scrollView.fullScroll(ScrollView.FOCUS_DOWN);
            }
        });

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MainActivity.this.marker = MainActivity.this.scatter.marker(graphData);
                MainActivity.this.marker.type(MarkerType.CIRCLE).size(1).color(modeCols[mode]);
            }
        });
    }

    public int getThreadCount() {
        return MainActivity.this.thread_count;
    }

    // append single line of text to the consoleDisplay
    public void appendToView(String newText) {
        final String text = "\n" + newText;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MainActivity.this.consoleDisplay.append(text);
            }
        });
    }

    // only outputs data to consoleDisplay and does not add to the chart
    public void dumpToView(double[] res) {
        final double[] results = res;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MainActivity.this.consoleDisplay.append("\n");
                for (int i = 0; i < results.length; i++) {
                    MainActivity.this.consoleDisplay.append(Integer.toString(i+1) + " [" + Double.toString(results[i]) + "]\n");
                }
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void executeCPP(int mode);
}

package com.snovi.browser;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.*;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Native shell; page parsing and requests are performed by the Snovi C++ engine. */
public final class MainActivity extends Activity {
    private final ExecutorService queue = Executors.newSingleThreadExecutor();
    private NativeEngine engine;
    private JSONObject state;
    private LinearLayout toolbar, content, links;
    private EditText address;
    private TextView page, status;
    private ProgressBar progress;
    private boolean busy, closed;
    private int blue=Color.rgb(30,102,211);

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        try {
            File ca=new File(getFilesDir(), "ca-bundle.pem");
            try(InputStream in=getAssets().open("ca-bundle.pem"); FileOutputStream out=new FileOutputStream(ca)) {
                byte[] buffer=new byte[8192]; int n; while((n=in.read(buffer))!=-1) out.write(buffer,0,n);
            }
            engine=new NativeEngine(ca.getAbsolutePath());
            state=engine.run("state", "");
            build(); render();
        } catch(Exception | UnsatisfiedLinkError e) {
            TextView error=new TextView(this); error.setText("Não foi possível iniciar o Snovi: " + e.getMessage()); error.setPadding(24,50,24,24); setContentView(error);
        }
    }
    private int dp(int n) { return (int)(n*getResources().getDisplayMetrics().density+0.5f); }
    private Button button(String title, Runnable action) {
        Button b=new Button(this); b.setText(title); b.setAllCaps(false); b.setTextColor(blue); b.setOnClickListener(v -> action.run()); return b;
    }
    private void build() {
        LinearLayout root=new LinearLayout(this); root.setOrientation(LinearLayout.VERTICAL); root.setBackgroundColor(Color.rgb(242,247,252));
        root.setPadding(dp(8),dp(32),dp(8),dp(8)); setContentView(root);
        TextView brand=new TextView(this); brand.setText("SNOVI  /  BROWSE BEYOND"); brand.setTextColor(blue); brand.setTypeface(null,Typeface.BOLD); brand.setPadding(dp(8),dp(4),0,dp(8)); root.addView(brand);
        toolbar=new LinearLayout(this); toolbar.setOrientation(LinearLayout.HORIZONTAL); root.addView(toolbar);
        toolbar.addView(button("‹",()->run("back",""))); toolbar.addView(button("›",()->run("forward","")));
        toolbar.addView(button("⟳",()->run("reload",""))); toolbar.addView(button("⌂",()->run("home","")));
        toolbar.addView(button("Abas",this::tabs)); toolbar.addView(button("⋮",this::menu));
        address=new EditText(this); address.setSingleLine(true); address.setHint("Pesquisar ou digitar endereço"); address.setTextSize(14); address.setImeOptions(EditorInfo.IME_ACTION_GO); root.addView(address);
        address.setOnEditorActionListener((v,id,event)-> { if(id==EditorInfo.IME_ACTION_GO || id==EditorInfo.IME_ACTION_DONE || (event!=null && event.getKeyCode()==KeyEvent.KEYCODE_ENTER && event.getAction()==KeyEvent.ACTION_UP)) {navigate(address.getText().toString());return true;} return false; });
        progress=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal); progress.setIndeterminate(true); progress.setVisibility(android.view.View.GONE); root.addView(progress,new LinearLayout.LayoutParams(-1,dp(3)));
        ScrollView scroll=new ScrollView(this); scroll.setFillViewport(true); root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        content=new LinearLayout(this); content.setBackgroundColor(Color.WHITE); content.setOrientation(LinearLayout.VERTICAL); content.setPadding(dp(20),dp(20),dp(20),dp(20)); scroll.addView(content);
        page=new TextView(this); page.setTextColor(Color.rgb(33,52,74)); page.setTextSize(16); page.setTextIsSelectable(true); content.addView(page);
        links=new LinearLayout(this); links.setOrientation(LinearLayout.VERTICAL); content.addView(links);
        status=new TextView(this); status.setTextSize(11); status.setTextColor(Color.GRAY); status.setPadding(dp(5),dp(8),0,0); root.addView(status);
    }
    private void navigate(String input) {
        String value=input.trim(); if(value.isEmpty()) return;
        if(!value.contains("://")) {
            if(!value.contains(" ") && (value.contains(".") || value.startsWith("localhost"))) value="https://"+value;
            else try { value="https://duckduckgo.com/?q="+URLEncoder.encode(value,"UTF-8"); } catch(Exception e) { return; }
        }
        run("navigate",value);
    }
    private void setBusy(boolean value) {
        busy=value; progress.setVisibility(value ? android.view.View.VISIBLE : android.view.View.GONE);
        address.setEnabled(!value); for(int i=0;i<toolbar.getChildCount();i++) toolbar.getChildAt(i).setEnabled(!value);
    }
    private void run(String action,String value) {
        if(busy || closed || engine==null) return;
        setBusy(true);
        queue.execute(()->{
            JSONObject next=null; String failure=null;
            try { next=engine.run(action,value); } catch(Exception e) { failure=e.getMessage(); }
            JSONObject result=next; String error=failure;
            runOnUiThread(()->{
                if(closed) return;
                if(result!=null) {state=result; render(); if(!result.optString("error").isEmpty()) error(result.optString("error"));}
                else error(error==null ? "Falha desconhecida" : error);
                setBusy(false);
            });
        });
    }
    private void render() {
        String url=state.optString("url", ""); address.setText(url); links.removeAllViews();
        if(url.isEmpty()) {
            page.setText("SNOVI\nBrowse beyond.\n\nDigite um endereço acima para começar.\n\nA renderização é textual nesta versão experimental.");
        } else {
            page.setText(state.optString("text", ""));
            JSONArray list=state.optJSONArray("links");
            if(list!=null && list.length()>0) {
                TextView title=new TextView(this); title.setText("\nLinks da página"); title.setTypeface(null,Typeface.BOLD); title.setTextColor(blue); links.addView(title);
                for(int i=0;i<Math.min(list.length(),100);i++) { final int index=i; String href=list.optString(i); Button link=button((i+1)+"  "+href,()->run("link",String.valueOf(index))); link.setGravity(Gravity.START|Gravity.CENTER_VERTICAL); links.addView(link); }
                if(list.length()>100) { TextView note=new TextView(this); note.setText("Mostrando os primeiros 100 links"); links.addView(note); }
            }
        }
        status.setText("MOTOR EXPERIMENTAL · Modo texto · Zoom "+state.optInt("zoom",100)+"% · "+state.optInt("blocked",0)+" requisições bloqueadas");
    }
    private void error(String message) { new AlertDialog.Builder(this).setTitle("Snovi").setMessage(message).setPositiveButton("OK",null).show(); }
    private void tabs() {
        JSONArray list=state.optJSONArray("tabs"); if(list==null) return;
        String[] names=new String[list.length()+1];
        for(int i=0;i<list.length();i++) names[i]=(i==state.optInt("currentTab")?"● ":"")+ (list.isNull(i)?"Nova aba":list.optString(i));
        names[list.length()]="＋ Nova aba";
        new AlertDialog.Builder(this).setTitle("Abas").setItems(names,(d,which)-> {if(which==list.length()) run("new_tab","");else run("select_tab",String.valueOf(which));}).setNegativeButton("Fechar aba atual",(d,w)->run("close_tab",String.valueOf(state.optInt("currentTab")))).show();
    }
    private void menu() {
        String[] options={"Nova aba","Atualizar página","Ver código-fonte","Compartilhar","Copiar endereço","Zoom +","Zoom −","Restaurar zoom","Favorito","Favoritos","Histórico","Ativar/desativar filtro","Sobre"};
        new AlertDialog.Builder(this).setTitle("Menu Snovi").setItems(options,(d,i)->{
            String url=state.optString("url", ""); int z=state.optInt("zoom",100);
            switch(i) {
                case 0: run("new_tab",""); break; case 1: run("reload",""); break;
                case 2: if(!url.isEmpty()) showText("Código-fonte · "+url,state.optString("source")); break;
                case 3: if(!url.isEmpty()) { Intent send=new Intent(Intent.ACTION_SEND); send.setType("text/plain"); send.putExtra(Intent.EXTRA_TEXT,url); startActivity(Intent.createChooser(send,"Compartilhar página")); } break;
                case 4: if(!url.isEmpty()) { ((ClipboardManager)getSystemService(CLIPBOARD_SERVICE)).setPrimaryClip(ClipData.newPlainText("Endereço",url)); Toast.makeText(this,"Endereço copiado",Toast.LENGTH_SHORT).show(); } break;
                case 5: run("zoom",String.valueOf(z+10)); break; case 6: run("zoom",String.valueOf(z-10)); break; case 7: run("zoom","100"); break;
                case 8: run("favorite",""); break; case 9: urls("Favoritos",state.optJSONArray("favorites")); break; case 10: urls("Histórico",state.optJSONArray("history")); break;
                case 11: run("adblock",""); break;
                case 12: error("Snovi experimental. Conteúdo em texto; sem CSS, JavaScript ou imagens. Filtro de rede com lista vazia; não bloqueia anúncios de sites modernos."); break;
            }
        }).show();
    }
    private void showText(String title,String text) {
        TextView view=new TextView(this); view.setText(text); view.setTextIsSelectable(true); view.setTypeface(Typeface.MONOSPACE); view.setPadding(dp(16),dp(16),dp(16),dp(16)); ScrollView scroll=new ScrollView(this); scroll.addView(view); new AlertDialog.Builder(this).setTitle(title).setView(scroll).setPositiveButton("Fechar",null).show();
    }
    private void urls(String title,JSONArray values) {
        if(values==null || values.length()==0) {error("Nenhuma página nesta sessão."); return;}
        String[] items=new String[values.length()]; for(int i=0;i<items.length;i++) items[i]=values.optString(i);
        new AlertDialog.Builder(this).setTitle(title).setItems(items,(d,i)->navigate(items[i])).setNegativeButton("Fechar",null).show();
    }
    @Override protected void onDestroy() {
        closed=true;
        if(engine!=null) { NativeEngine old=engine; queue.execute(old::close); }
        queue.shutdown(); super.onDestroy();
    }
}

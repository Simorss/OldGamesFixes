// OldGamesFixes: language-independent Vision 7.1 x86 engine recognition.
// No executable-name, game-language, edition or whole-file hash allowlist.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OldGamesFixesV2 {
    public sealed class Section {
        public int Raw, Size; public uint Rva, Flags;
    }
    public sealed class Layout {
        public byte[] Baseline;
        public uint ImageBase, RootInitializer;
        public int GameplayAspect, ForwardX, ForwardY, InverseX, InverseY;
        public int WidthConstant, HeightConstant, ReciprocalWidth, ReciprocalHeight;
        public int MouseWidth, MouseHeight, ClipWidth, ClipHeight, RootCall;
        public int[] VerticalReferences;
        public float PreviousAspect;
        public string EngineFamily = "Vision 7.1 / MSVC x86 GUI";
    }
    public sealed class Image {
        public readonly byte[] Bytes;
        public readonly uint Base;
        public readonly List<Section> Sections = new List<Section>();
        public readonly Dictionary<uint,string> Imports = new Dictionary<uint,string>();
        void Range(int at, int n) {
            if (at < 0 || n < 0 || (long)at+n > Bytes.Length) throw new Exception("Invalid PE range.");
        }
        public ushort U16(int at) { Range(at,2); return BitConverter.ToUInt16(Bytes,at); }
        public uint U32(int at) { Range(at,4); return BitConverter.ToUInt32(Bytes,at); }
        public float F32(int at) { Range(at,4); return BitConverter.ToSingle(Bytes,at); }
        public int Rva(uint rva) {
            foreach (Section s in Sections)
                if (rva >= s.Rva && (ulong)rva-s.Rva < (uint)s.Size) return checked(s.Raw+(int)(rva-s.Rva));
            throw new Exception("A required PE address has no file-backed section.");
        }
        public int Offset(uint va) { if (va < Base) throw new Exception("Invalid engine pointer."); return Rva(va-Base); }
        public uint Address(int off) {
            foreach (Section s in Sections)
                if (off >= s.Raw && off-s.Raw < s.Size) return checked(Base+s.Rva+(uint)(off-s.Raw));
            throw new Exception("Patch site is outside PE sections.");
        }
        public string Z(int at) {
            Range(at,1); int end=at;
            while (end < Bytes.Length && end-at < 512 && Bytes[end]!=0) end++;
            if (end==Bytes.Length || end-at==512) throw new Exception("Invalid PE import name.");
            return Encoding.ASCII.GetString(Bytes,at,end-at);
        }
        public Image(byte[] bytes) {
            Bytes=bytes; Range(0,64);
            if (U16(0)!=0x5a4d) throw new Exception("Not a Windows executable.");
            int pe=checked((int)U32(0x3c)); Range(pe,24);
            if (U32(pe)!=0x4550 || U16(pe+4)!=0x14c) throw new Exception("The patch requires a 32-bit x86 PE executable.");
            int count=U16(pe+6), optional=pe+24, optSize=U16(pe+20);
            if (count<1 || count>96 || optSize<112 || U16(optional)!=0x10b) throw new Exception("Unsupported PE header.");
            Base=U32(optional+28);
            for (int n=0;n<count;n++) {
                int at=optional+optSize+40*n; Range(at,40);
                Section s=new Section { Rva=U32(at+12), Size=checked((int)U32(at+16)), Raw=checked((int)U32(at+20)), Flags=U32(at+36) };
                Range(s.Raw,s.Size); Sections.Add(s);
            }
            uint imports=U32(optional+104);
            if (imports==0) throw new Exception("No engine imports found; packed executables need a separately supported profile.");
            int desc=Rva(imports);
            for (int n=0;n<512;n++,desc+=20) {
                uint names=U32(desc), dll=U32(desc+12), first=U32(desc+16);
                if (names==0 && dll==0 && first==0) break;
                string library=Z(Rva(dll)); int lookup=Rva(names!=0?names:first);
                for (int j=0;j<8192;j++) {
                    uint entry=U32(lookup+j*4); if (entry==0) break;
                    if ((entry&0x80000000)==0) Imports[Base+first+(uint)(4*j)]=library+"!"+Z(Rva(entry)+2);
                }
            }
        }
        public int[] Find(string signature) {
            string[] parts=signature.Split(new[]{' '},StringSplitOptions.RemoveEmptyEntries);
            int[] sig=parts.Select(x=>x=="??"?-1:Convert.ToInt32(x,16)).ToArray();
            List<int> hits=new List<int>();
            foreach (Section section in Sections) {
                if ((section.Flags&0x20000000)==0) continue;
                int end=section.Raw+section.Size-sig.Length;
                for (int at=section.Raw;at<=end;at++) {
                    if (sig[0]>=0 && Bytes[at]!=sig[0]) continue;
                    int k=1; while(k<sig.Length && (sig[k]<0 || Bytes[at+k]==sig[k])) k++;
                    if (k==sig.Length) hits.Add(at);
                }
            }
            return hits.ToArray();
        }
        public uint BranchTarget(int at) {
            Range(at,5);
            if (Bytes[at]!=0xe8 && Bytes[at]!=0xe9) throw new Exception("Expected a relative x86 branch.");
            return unchecked((uint)(Address(at)+5+BitConverter.ToInt32(Bytes,at+1)));
        }
        public bool ImportCall(int at,string name) {
            try {
                int target=Offset(BranchTarget(at));
                if(Bytes[target]!=0xff || Bytes[target+1]!=0x25) return false;
                string imported;
                return Imports.TryGetValue(U32(target+2),out imported) && imported.IndexOf(name,StringComparison.Ordinal)>=0;
            } catch { return false; }
        }
    }
    public static class Engine {
        static int One(IEnumerable<int> values,string label) {
            int[] list=values.Distinct().ToArray();
            if(list.Length!=1) throw new Exception("Engine recognition failed: "+label+" has "+list.Length+" matches. No file was changed.");
            return list[0];
        }
        static bool Equal(byte[] b,int at,params byte[] expected) {
            if(at<0 || at+expected.Length>b.Length) return false;
            for(int i=0;i<expected.Length;i++) if(b[at+i]!=expected[i]) return false;
            return true;
        }
        static bool FinitePositive(float n) { return !Single.IsNaN(n) && !Single.IsInfinity(n) && n>0; }
        static void Put(byte[] b,int at,byte[] value) { Array.Copy(value,0,b,at,value.Length); }
        static void Float(byte[] b,int at,double value) { Put(b,at,BitConverter.GetBytes((float)value)); }
        static byte[] Branch(byte op,uint from,uint to) {
            byte[] result=new byte[5]; result[0]=op;
            Put(result,1,BitConverter.GetBytes(unchecked((int)(to-from-5)))); return result;
        }
        static bool Padding(byte[] b,int at,int size) {
            if(at<0 || at+size>b.Length) return false;
            for(int i=0;i<size;i++) if(b[at+i]!=0xcc) return false;
            return true;
        }
        static bool StubPadding(byte[] b,int at,byte stack) {
            return Padding(b,at,15) || (Equal(b,at,0xc7,0x44,0x24,stack) && b[at+8]==0xe9 && Padding(b,at+13,2));
        }
        static bool PointerUse(byte[] b,int at) {
            return Equal(b,at-2,0xd8,0x0d) || (at>=4 && b[at-4]==0xf3 && b[at-3]==0x0f &&
                (b[at-2]==0x10 || b[at-2]==0x59) && (b[at-1]&0xc7)==5);
        }
        static uint ResolveInitializer(Image p,int rootCall) {
            uint target=p.BranchTarget(rootCall);
            for(int n=0;n<3;n++) {
                int off=p.Offset(target);
                if(Equal(p.Bytes,off,0xc7,0x44,0x24,0x04) || Equal(p.Bytes,off,0xc7,0x44,0x24,0x08)) {
                    target=p.BranchTarget(off+8); continue;
                }
                if(!Equal(p.Bytes,off,0x83,0xc1,0x30,0xe9)) throw new Exception("Unknown root GUI initializer.");
                return target;
            }
            throw new Exception("Unknown GUI translation trampoline.");
        }
        public static Layout Inspect(byte[] bytes) {
            Image p=new Image(bytes); Layout l=new Layout(); l.ImageBase=p.Base;
            int gameplay=One(p.Find("A1 ?? ?? ?? ?? 68 ?? ?? ?? ?? 8D 4C 24 ?? 51 8B 0D ?? ?? ?? ?? 8D 54 24 ?? 52 50 E8 ?? ?? ?? ?? 8B 4C 24 ?? 8B 54 24 ?? 51 52 E8")
                .Where(x=>p.ImportCall(x+27,"PerspectiveToAngles@VisRenderer_cl")),"main camera projection");
            l.GameplayAspect=gameplay+6; l.PreviousAspect=p.F32(l.GameplayAspect);
            if(!FinitePositive(l.PreviousAspect)) throw new Exception("Invalid camera aspect parameter.");
            int root=One(p.Find("68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 53 53 8B CF E8 ?? ?? ?? ?? 53 53 8B CD E8 ?? ?? ?? ?? 68 00 00 40 44 68 00 00 80 44 53 53 8B CD E8"),"root GUI constructor");
            l.ClipHeight=root+1; l.ClipWidth=root+6; l.RootCall=root+23;
            l.RootInitializer=ResolveInitializer(p,l.RootCall);
            int setter=p.Offset(p.BranchTarget(p.Offset(l.RootInitializer)+3));
            int[] fw=p.Find("D9 05 ?? ?? ?? ?? D8 4C 24 04 D8 0D ?? ?? ?? ?? C3");
            int[] inv=p.Find("D9 44 24 04 D8 35 ?? ?? ?? ?? D8 0D ?? ?? ?? ?? C3");
            List<int> groups=new List<int>();
            foreach(int f in fw) {
                int[] near=fw.Where(x=>x>f && x-f<128).ToArray();
                foreach(int y in near) {
                    int[] ix=inv.Where(x=>x>y && x-y<128 && p.U32(x+6)==p.U32(f+2)).ToArray();
                    if(ix.Length!=1) continue;
                    int[] iy=inv.Where(x=>x>ix[0] && x-ix[0]<128 && p.U32(x+6)==p.U32(y+2)).ToArray();
                    if(iy.Length==1 && setter>iy[0] && setter-iy[0]<0x10000) groups.Add(f);
                }
            }
            l.ForwardX=One(groups,"GUI coordinate conversion family");
            l.ForwardY=One(fw.Where(x=>x>l.ForwardX && x-l.ForwardX<128),"vertical forward conversion");
            uint globalW=p.U32(l.ForwardX+2), globalH=p.U32(l.ForwardY+2);
            l.InverseX=One(inv.Where(x=>x>l.ForwardY && x-l.ForwardY<128 && p.U32(x+6)==globalW),"horizontal inverse conversion");
            l.InverseY=One(inv.Where(x=>x>l.InverseX && x-l.InverseX<128 && p.U32(x+6)==globalH),"vertical inverse conversion");
            l.WidthConstant=p.Offset(p.U32(l.InverseX+12));
            l.HeightConstant=p.Offset(p.U32(l.InverseY+12));
            l.ReciprocalWidth=p.Offset(p.U32(l.ForwardX+12));
            uint currentY=p.U32(l.ForwardY+12);
            bool oldPrivateY=p.Offset(currentY)==l.InverseX+17;
            if(!oldPrivateY) l.ReciprocalHeight=p.Offset(currentY);
            else {
                List<int> values=new List<int>(); byte[] target=BitConverter.GetBytes(1f/768f);
                foreach(Section section in p.Sections) {
                    if((section.Flags&0x20000000)!=0) continue;
                    for(int at=section.Raw;at<=section.Raw+section.Size-4;at+=4)
                        if(Equal(bytes,at,target)) values.Add(at);
                }
                l.ReciprocalHeight=One(values,"original vertical reciprocal for legacy migration");
            }
            if(!FinitePositive(p.F32(l.WidthConstant)) || !FinitePositive(p.F32(l.HeightConstant)) ||
                Math.Abs(p.F32(l.WidthConstant)*p.F32(l.ReciprocalWidth)-1)>0.001 ||
                Math.Abs(p.F32(l.HeightConstant)*p.F32(p.Offset(currentY))-1)>0.001)
                throw new Exception("Inconsistent GUI scale constants.");
            int mouse=One(p.Find("F3 0F 2A 04 24 F3 0F 59 05 ?? ?? ?? ?? F3 0F 5E 05 ?? ?? ?? ?? 8B 4C 24 0C 8B 54 24 10 F3 0F 11 01 F3 0F 2A 44 24 04 F3 0F 59 05 ?? ?? ?? ?? F3 0F 5E 05 ?? ?? ?? ?? F3 0F 11 02 83 C4 08 C2 08 00")
                .Where(x=>p.U32(x+17)==globalW && p.U32(x+51)==globalH),"GUI mouse coordinate mapping");
            l.MouseWidth=p.Offset(p.U32(mouse+9)); l.MouseHeight=p.Offset(p.U32(mouse+43));
            if(Math.Abs(p.F32(l.MouseWidth)-p.F32(l.WidthConstant))>0.01 ||
                Math.Abs(p.F32(l.MouseHeight)-p.F32(l.HeightConstant))>0.01)
                throw new Exception("GUI mouse and drawing dimensions disagree.");
            if(!StubPadding(bytes,l.ForwardX+17,4) || !StubPadding(bytes,l.ForwardY+17,8) ||
                !(Padding(bytes,l.InverseX+17,15) || (oldPrivateY && Padding(bytes,l.InverseX+21,11))))
                throw new Exception("Required GUI alignment space is already used by an unknown patch.");
            List<int> refs=new List<int>();
            for(int at=l.ForwardX;at<setter-4;at++) if(p.U32(at)==currentY) {
                if(!PointerUse(bytes,at)) throw new Exception("Unknown vertical GUI pointer instruction.");
                refs.Add(at);
            }
            if(refs.Count<2) throw new Exception("Vertical GUI references could not be identified.");
            l.VerticalReferences=refs.ToArray(); l.Baseline=(byte[])bytes.Clone();
            Float(l.Baseline,l.GameplayAspect,4.0/3.0);
            foreach(int at in new[]{l.WidthConstant,l.MouseWidth,l.ClipWidth}) Float(l.Baseline,at,1024);
            foreach(int at in new[]{l.HeightConstant,l.MouseHeight,l.ClipHeight}) Float(l.Baseline,at,768);
            Float(l.Baseline,l.ReciprocalWidth,1.0/1024); Float(l.Baseline,l.ReciprocalHeight,1.0/768);
            foreach(int at in refs) Put(l.Baseline,at,BitConverter.GetBytes(p.Address(l.ReciprocalHeight)));
            foreach(int at in new[]{l.ForwardX+17,l.ForwardY+17,l.InverseX+17})
                for(int j=0;j<15;j++) l.Baseline[at+j]=0xcc;
            Put(l.Baseline,l.RootCall,Branch(0xe8,p.Address(l.RootCall),l.RootInitializer));
            return l;
        }
        public static byte[] Build(Layout l,int width,int height,bool originalInterface) {
            byte[] b=(byte[])l.Baseline.Clone(); Image p=new Image(b);
            Float(b,l.GameplayAspect,(double)width/height);
            if(!originalInterface) return b;
            double scale=Math.Min(width/1024.0,height/768.0), cw=width/scale, ch=height/scale;
            foreach(int at in new[]{l.WidthConstant,l.MouseWidth,l.ClipWidth}) Float(b,at,cw);
            foreach(int at in new[]{l.HeightConstant,l.MouseHeight,l.ClipHeight}) Float(b,at,ch);
            Float(b,l.ReciprocalWidth,1.0/cw);
            int sx=l.ForwardX+17, sy=l.ForwardY+17, reciprocal=l.InverseX+17;
            Float(b,reciprocal,1.0/ch);
            foreach(int at in l.VerticalReferences) Put(b,at,BitConverter.GetBytes(p.Address(reciprocal)));
            Put(b,l.RootCall,Branch(0xe8,p.Address(l.RootCall),p.Address(sx)));
            Put(b,sx,new byte[]{0xc7,0x44,0x24,4}); Float(b,sx+4,(cw-1024)/2);
            Put(b,sx+8,Branch(0xe9,p.Address(sx+8),p.Address(sy)));
            Put(b,sy,new byte[]{0xc7,0x44,0x24,8}); Float(b,sy+4,(ch-768)/2);
            Put(b,sy+8,Branch(0xe9,p.Address(sy+8),l.RootInitializer));
            return b;
        }
    }
}

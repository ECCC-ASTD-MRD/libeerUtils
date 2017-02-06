#!/bin/sh
# the next line restarts using tclsh \
exec $SPI_PATH/tclsh "$0" "$@"
#============================================================================
# Environnement Canada
# Centre Meteorologique Canadien
# 2100 Trans-Canadienne
# Dorval, Quebec
#
# Projet     : Exemple de scripts.
# Fichier    : Dict_CheckTree.tcl
# Creation   : Janvier 2017 - J.P. Gauthier - CMC/CMOE
# Description: Stats on variables in the $CMCGRIDF
#
# Parametres :
#
# Retour:
#
# Remarques  :
#
#============================================================================

package require Logger
package require TclData

namespace eval Dict {
   variable Param
   variable Dict

   set Param(Version) 0.1

   fstddict load
   set Param(Vars) [fstddict var]

   #----- Initialize var counter
   foreach var $Param(Vars) {
      set Dict([string trim $var]) 0
   }
}

proc Dict::ParseFiles { Files { Recursive False } } {
   variable Param
   variable Dict
   variable Path
   
   #----- Loop on files
   foreach file $Files {
      if { [string first ":" $file]!=-1 } {
         return
      }
      
      Log::Print INFO "Checking $file"
      
      if { [file isdirectory $file] && $Recursive } {
         Dict::ParseFiles [glob -nocomplain $file/*] True
      }
   
      #----- Loop on file's var
      set vars {}
      catch { set vars [fstdfile open FILE read $file NOMVAR] }
      foreach var $vars {
         if { ![fstddict isvar $var] } {
            lappend Path($var) [file dirname $file]
         }
         incr Dict($var)
      }
      catch { fstdfile close FILE }
   }  
}

proc Dict::Stats { } {
   variable Param
   variable Dict
   variable Path

   #----- Loop on all vars
   foreach var [array names Dict] {
      #----- Check if it is defined in the dicitonnary
      if { [lsearch -exact $Param(Vars) "$var"]==-1 } {
         lappend notdefined $var
      } else {
         #----- Check if we found any occurences of it
         if { !$Dict($var) } {
            lappend notfound $var
         }
      }
   }
     
   Log::Print MUST "\nVariables not defined ([llength $notdefined]):\n"
   foreach var $notdefined {
      puts "\t$var: [lsort -unique $Path($var)]"
   }
   Log::Print MUST "\nVariables defined but not found ([llength $notfound]):\n\t$notfound"
   
}
   
Log::Start [file tail [info script]] $Dict::Param(Version)

Dict::ParseFiles $argv True
Dict::Stats

Log::End 0

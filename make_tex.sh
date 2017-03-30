#!/bin/bash

chip=$1
dose=$2

echo "
\section{Chip${chip} ${dose}kGy}

\subsection{Pedestal \& Noise}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Fe0CBC0_Pedestal_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Fe0CBC0_Noise_Chip${chip}_${dose}kGy\".png}
\caption{Pedestal and Noise extracted after offset tuning.}
\end{figure}

\subsection{SCurves}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/SCurves_TP0_Chip${chip}_${dose}kGy\".png}
\caption{SCurves without Test Pulse}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/SCurves_TP225_Chip${chip}_${dose}kGy\".png}
\caption{SCurves with Test Pulse=30}
\end{figure}

\subsection{Digital Current}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ch4I_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Minimal Digital Current_Chip${chip}_${dose}kGy\".png}
\caption{VDDD and Minimal digital current.}
\end{figure}

\subsection{Single Measurements}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VDDA_Chip${chip}_${dose}kGy\".png}
\caption{VDDA}
\end{figure}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ibias_Chip${chip}_${dose}kGy\".png}
\caption{I bias}
\end{figure}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Nc50_Chip${chip}_${dose}kGy\".png}
\caption{Nc50}
\end{figure}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VBG_LDO_Chip${chip}_${dose}kGy\".png}
\caption{VBG LDO}
\end{figure}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Vpafb_Chip${chip}_${dose}kGy\".png}
\caption{Vpafb}
\end{figure}

\subsection{Bias Sweeps}
Bias sweeps and DMM measurements at the nominal DAC value of the bias are shown together. The legend contains one entry for each sweep. The Graph colors are coded: every 10\% range of the dose has it's own color.
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Legend_Chip${chip}_${dose}kGy\".png}
\caption{Legend showing all measurements with doese}
\end{figure}

\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/CAL_I_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/CAL_I_sweep_Chip${chip}_${dose}kGy\".png}
\caption{CAL I}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/CAL_VCasc_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/CAL_VCasc_sweep_Chip${chip}_${dose}kGy\".png}
\caption{CAL VCasc}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Icomp_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Icomp_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Icomp}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ihyst_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ihyst_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ihyst}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipa_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipa_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ipa}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipaos_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipaos_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ipaos}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipre1_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipre1_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ipre1}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipre2_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipre2_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ipre2}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipsf_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/Ipsf_sweep_Chip${chip}_${dose}kGy\".png}
\caption{Ipsf}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VBGbias_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VBGbias_sweep_Chip${chip}_${dose}kGy\".png}
\caption{V BG bias}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VCth_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VCth_sweep_Chip${chip}_${dose}kGy\".png}
\caption{VCth}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VPLUS1_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VPLUS1_sweep_Chip${chip}_${dose}kGy\".png}
\caption{V PLUS1}
\end{figure}
\begin{figure}[!htb]
\centering
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VPLUS2_Chip${chip}_${dose}kGy\".png}
\includegraphics[width=.8\textwidth]{\"Results/Chip${chip}_${dose}kGy/VPLUS2_sweep_Chip${chip}_${dose}kGy\".png}
\caption{V PLUS2}
\end{figure}
" > plots_chip${chip}_${dose}kGy.tex
